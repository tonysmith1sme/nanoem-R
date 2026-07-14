/*
   Copyright (c) 2015-2023 hkrn All rights reserved

   This file is licensed under MIT license. for more details, see LICENSE.txt.
 */

#include "fx9/GraphicsAPITranslator.h"

#include <algorithm>
#include <cstring>
#include <sstream>

#include "glslang/Public/ShaderLang.h"
#include "spirv_cross/spirv_glsl.hpp"
#include "spirv_cross/spirv_hlsl.hpp"
#include "spirv_cross/spirv_msl.hpp"

namespace fx9 {
namespace graphics {
namespace {

bool
hasBothStages(const TranslateRequest &request)
{
    return request.vertexSPIRV && request.fragmentSPIRV && !request.vertexSPIRV->empty() &&
        !request.fragmentSPIRV->empty();
}

void
optimizeStage(const HostServices &host, const SPIRVWords &inWords, SPIRVWords &outWords, ErrorSink &errors)
{
    if (host.optimize) {
        host.optimize(inWords, outWords, errors);
    }
    else {
        outWords = inWords;
    }
}

void
saveStageAttributes(const HostServices &host, const SPIRVWords &words, AttributeNameMap &attributes)
{
    if (host.saveAttributes) {
        host.saveAttributes(words, attributes);
    }
}

void
restoreIfNeeded(const HostServices &host, ShaderStageLanguage language, const AttributeNameMap &attributes,
    spirv_cross::Compiler &compiler)
{
    if (host.restoreAttributes) {
        host.restoreAttributes(language, attributes, &compiler);
    }
}

void
applySamplerBindingsGLSL(spirv_cross::CompilerGLSL &compiler, const SamplerBindingMap *samplers)
{
    if (!samplers) {
        return;
    }
    const spirv_cross::ShaderResources resources = compiler.get_shader_resources();
    for (const auto &image : resources.sampled_images) {
        auto it = samplers->find(image.name);
        if (it != samplers->end()) {
            compiler.set_decoration(image.id, spv::DecorationBinding, static_cast<uint32_t>(it->second));
        }
    }
}

void
applySamplerBindingsHLSL(spirv_cross::CompilerHLSL &compiler, const SamplerBindingMap *samplers)
{
    if (!samplers) {
        return;
    }
    const spirv_cross::ShaderResources resources = compiler.get_shader_resources();
    for (const auto &image : resources.sampled_images) {
        auto it = samplers->find(image.name);
        if (it != samplers->end()) {
            compiler.set_decoration(image.id, spv::DecorationBinding, static_cast<uint32_t>(it->second));
        }
    }
}

void
applySamplerBindingsMSL(spirv_cross::CompilerMSL &compiler, const SamplerBindingMap *samplers, spv::ExecutionModel stage)
{
    if (!samplers) {
        return;
    }
    const spirv_cross::ShaderResources resources = compiler.get_shader_resources();
    for (const auto &image : resources.sampled_images) {
        auto it = samplers->find(image.name);
        if (it != samplers->end()) {
            spirv_cross::MSLResourceBinding binding = {};
            binding.stage = stage;
            binding.binding = static_cast<uint32_t>(it->second);
            binding.msl_sampler = binding.msl_texture = static_cast<uint32_t>(it->second);
            compiler.add_msl_resource_binding(binding);
            compiler.set_decoration(image.id, spv::DecorationBinding, static_cast<uint32_t>(it->second));
        }
    }
}

/*
 * Patch SPIR-V OpDecorate Binding / DescriptorSet for sampled images listed in
 * the sampler map. This is the Vulkan path's binding contract for MME registers.
 */
bool
rewriteVulkanSamplerBindings(SPIRVWords &words, const SamplerBindingMap *samplers, uint32_t descriptorSet,
    ErrorSink &errors)
{
    if (!samplers || samplers->empty() || words.size() < 5) {
        return true;
    }
    try {
        spirv_cross::CompilerGLSL reflector(words);
        const spirv_cross::ShaderResources resources = reflector.get_shader_resources();
        for (const auto &image : resources.sampled_images) {
            auto it = samplers->find(image.name);
            if (it == samplers->end()) {
                continue;
            }
            reflector.set_decoration(image.id, spv::DecorationDescriptorSet, descriptorSet);
            reflector.set_decoration(image.id, spv::DecorationBinding, static_cast<uint32_t>(it->second));
        }
        /* SPIRV-Cross does not re-emit SPIR-V; apply patches by rebuilding via
           binary mutation of OpDecorate instructions using reflection ids. */
        std::unordered_map<uint32_t, uint32_t> idToBinding;
        std::unordered_map<uint32_t, uint32_t> idToSet;
        for (const auto &image : resources.sampled_images) {
            auto it = samplers->find(image.name);
            if (it != samplers->end()) {
                idToBinding[static_cast<uint32_t>(image.id)] = static_cast<uint32_t>(it->second);
                idToSet[static_cast<uint32_t>(image.id)] = descriptorSet;
            }
        }
        if (idToBinding.empty()) {
            return true;
        }
        /* SPIR-V header: magic, version, generator, bound, schema */
        size_t i = 5;
        while (i < words.size()) {
            const uint32_t instr = words[i];
            const uint32_t opcode = instr & spv::OpCodeMask;
            const uint32_t length = instr >> spv::WordCountShift;
            if (length == 0 || i + length > words.size()) {
                errors.insert("vulkan SPIR-V parse error while rewriting bindings");
                return false;
            }
            if (opcode == spv::OpDecorate && length >= 4) {
                const uint32_t target = words[i + 1];
                const uint32_t decoration = words[i + 2];
                if (decoration == spv::DecorationBinding) {
                    auto it = idToBinding.find(target);
                    if (it != idToBinding.end()) {
                        words[i + 3] = it->second;
                    }
                }
                else if (decoration == spv::DecorationDescriptorSet) {
                    auto it = idToSet.find(target);
                    if (it != idToSet.end()) {
                        words[i + 3] = it->second;
                    }
                }
            }
            i += length;
        }
        /* Ensure missing Binding/DescriptorSet decorations exist: if reflection
           saw the id but binary had no decorate, leave as-is (glslang emits them). */
        return true;
    } catch (const std::exception &e) {
        errors.insert(e.what());
        return false;
    }
}

std::vector<spirv_cross::HLSLVertexAttributeRemap>
buildDirectXVertexInputRemaps()
{
    /* Historical contract: descending from 0x7ff for POSITION..COLOR0. */
    std::vector<spirv_cross::HLSLVertexAttributeRemap> mapping;
    uint32_t offset = 0x7ff;
    mapping.push_back(spirv_cross::HLSLVertexAttributeRemap { offset--, "SV_Position" });
    mapping.push_back(spirv_cross::HLSLVertexAttributeRemap { offset--, "NORMAL" });
    mapping.push_back(spirv_cross::HLSLVertexAttributeRemap { offset--, "TEXCOORD0" });
    mapping.push_back(spirv_cross::HLSLVertexAttributeRemap { offset--, "TEXCOORD1" });
    mapping.push_back(spirv_cross::HLSLVertexAttributeRemap { offset--, "TEXCOORD2" });
    mapping.push_back(spirv_cross::HLSLVertexAttributeRemap { offset--, "TEXCOORD3" });
    mapping.push_back(spirv_cross::HLSLVertexAttributeRemap { offset--, "TEXCOORD4" });
    mapping.push_back(spirv_cross::HLSLVertexAttributeRemap { offset--, "COLOR0" });
    return mapping;
}

std::vector<spirv_cross::HLSLVertexAttributeRemap>
buildDirectXFragmentInputRemaps()
{
    std::vector<spirv_cross::HLSLVertexAttributeRemap> mapping;
    for (uint32_t i = 0; i < 16; i++) {
        std::ostringstream s;
        s << "TEXCOORD" << i;
        mapping.push_back(spirv_cross::HLSLVertexAttributeRemap { i, s.str() });
    }
    return mapping;
}

bool
translateOpenGLFamily(const TranslateRequest &request, const OpenGLOptions &options, const HostServices &host,
    TranslateResult &result, ErrorSink &errors, GraphicsAPI api)
{
    result = TranslateResult();
    result.api = api;
    if (!hasBothStages(request)) {
        return false;
    }
    try {
        auto compileStage = [&](ShaderStageLanguage language, const SPIRVWords &words,
                                const SamplerBindingMap *samplers) {
            SPIRVWords optimized;
            AttributeNameMap attributes;
            saveStageAttributes(host, words, attributes);
            optimizeStage(host, words, optimized, errors);
            spirv_cross::CompilerGLSL compiler(optimized);
            configureOpenGLBackend(&compiler, options);
            applySamplerBindingsGLSL(compiler, samplers);
            restoreIfNeeded(host, language, attributes, compiler);
            return compiler.compile();
        };
        result.vertexSource = compileStage(
            static_cast<ShaderStageLanguage>(EShLangVertex), *request.vertexSPIRV, request.vertexSamplers);
        result.fragmentSource = compileStage(
            static_cast<ShaderStageLanguage>(EShLangFragment), *request.fragmentSPIRV, request.fragmentSamplers);
        result.succeeded = true;
    } catch (const spirv_cross::CompilerError &e) {
        errors.insert(e.what());
    } catch (const std::exception &e) {
        errors.insert(e.what());
    }
    return result.succeeded;
}

} /* namespace anonymous */

const char *
graphicsAPIName(GraphicsAPI api)
{
    switch (api) {
    case GraphicsAPI::OpenGL:
        return "OpenGL";
    case GraphicsAPI::OpenGLES:
        return "OpenGLES";
    case GraphicsAPI::DirectX:
        return "DirectX";
    case GraphicsAPI::Metal:
        return "Metal";
    case GraphicsAPI::Vulkan:
        return "Vulkan";
    default:
        return "Unknown";
    }
}

const char *
metalShaderPreamble()
{
    return "#pragma clang diagnostic ignored \"-Wunused-variable\"\n"
           "#include <metal_math>\n"
           "using namespace metal;\n"
           "float length(float x) { return sqrt(x * x); }\n";
}

void
configureOpenGLBackend(void *compilerGLSL, const OpenGLOptions &options)
{
    auto &compiler = *static_cast<spirv_cross::CompilerGLSL *>(compilerGLSL);
    spirv_cross::CompilerGLSL::Options opts;
    opts.enable_420pack_extension = false;
    opts.es = options.es;
    opts.version = static_cast<uint32_t>(options.version);
    opts.vertex.fixup_clipspace = true;
    opts.fragment.default_float_precision = spirv_cross::CompilerGLSL::Options::Highp;
    opts.fragment.default_int_precision = spirv_cross::CompilerGLSL::Options::Highp;
    compiler.set_common_options(opts);
}

void
configureDirectXBackend(void *compilerHLSL, const DirectXOptions &options)
{
    auto &compiler = *static_cast<spirv_cross::CompilerHLSL *>(compilerHLSL);
    spirv_cross::CompilerHLSL::Options opts;
    opts.shader_model = options.shaderModel;
    opts.point_size_compat = true;
    opts.point_coord_compat = true;
    compiler.set_hlsl_options(opts);
}

void
configureMetalBackend(void *compilerMSL, const MetalOptions &options)
{
    auto &compiler = *static_cast<spirv_cross::CompilerMSL *>(compilerMSL);
    spirv_cross::CompilerMSL::Options opts;
    opts.platform = spirv_cross::CompilerMSL::Options::macOS;
    opts.set_msl_version(options.major, options.minor);
    opts.pad_fragment_output_components = true;
    opts.enable_decoration_binding = true;
    opts.force_native_arrays = true;
    compiler.set_msl_options(opts);
    (void) options.entryPoint;
    (void) options.uniformBufferName;
}

bool
translateOpenGL(const TranslateRequest &request, const OpenGLOptions &options, const HostServices &host,
    TranslateResult &result, ErrorSink &errors)
{
    OpenGLOptions opts = options;
    opts.es = false;
    return translateOpenGLFamily(request, opts, host, result, errors, GraphicsAPI::OpenGL);
}

bool
translateOpenGLES(const TranslateRequest &request, const OpenGLOptions &options, const HostServices &host,
    TranslateResult &result, ErrorSink &errors)
{
    OpenGLOptions opts = options;
    opts.es = true;
    if (opts.version < 300) {
        opts.version = 300;
    }
    return translateOpenGLFamily(request, opts, host, result, errors, GraphicsAPI::OpenGLES);
}

bool
translateDirectX(const TranslateRequest &request, const DirectXOptions &options, const HostServices &host,
    TranslateResult &result, ErrorSink &errors)
{
    result = TranslateResult();
    result.api = GraphicsAPI::DirectX;
    if (!hasBothStages(request)) {
        return false;
    }
    try {
        auto compileStage = [&](const SPIRVWords &words,
                                const std::vector<spirv_cross::HLSLVertexAttributeRemap> &mapping,
                                const SamplerBindingMap *samplers) {
            SPIRVWords optimized;
            optimizeStage(host, words, optimized, errors);
            spirv_cross::CompilerHLSL compiler(optimized);
            configureDirectXBackend(&compiler, options);
            applySamplerBindingsHLSL(compiler, samplers);
            for (const auto &remap : mapping) {
                compiler.add_vertex_attribute_remap(remap);
            }
            std::ostringstream stream;
            stream << compiler.compile();
            return stream.str();
        };
        result.vertexSource =
            compileStage(*request.vertexSPIRV, buildDirectXVertexInputRemaps(), request.vertexSamplers);
        result.fragmentSource =
            compileStage(*request.fragmentSPIRV, buildDirectXFragmentInputRemaps(), request.fragmentSamplers);
        result.succeeded = true;
    } catch (const spirv_cross::CompilerError &e) {
        errors.insert(e.what());
    } catch (const std::exception &e) {
        errors.insert(e.what());
    }
    return result.succeeded;
}

bool
translateMetal(const TranslateRequest &request, const MetalOptions &options, const HostServices &host,
    TranslateResult &result, ErrorSink &errors)
{
    result = TranslateResult();
    result.api = GraphicsAPI::Metal;
    if (!hasBothStages(request)) {
        return false;
    }
    try {
        auto compileStage = [&](ShaderStageLanguage language, const SPIRVWords &words,
                                const SamplerBindingMap *samplers) {
            SPIRVWords optimized;
            AttributeNameMap attributes;
            saveStageAttributes(host, words, attributes);
            optimizeStage(host, words, optimized, errors);
            spirv_cross::CompilerMSL compiler(optimized);
            const spv::ExecutionModel stage =
                language == EShLangVertex ? spv::ExecutionModelVertex : spv::ExecutionModelFragment;
            const auto &entryPoints = compiler.get_entry_points_and_stages();
            auto it = std::find_if(entryPoints.begin(), entryPoints.end(),
                [&](const spirv_cross::EntryPoint &item) { return item.execution_model == stage; });
            if (it != entryPoints.end()) {
                compiler.rename_entry_point(it->name, options.entryPoint, stage);
            }
            configureMetalBackend(&compiler, options);
            applySamplerBindingsMSL(compiler, samplers, stage);
            spirv_cross::ShaderResources resources = compiler.get_shader_resources();
            const spirv_cross::SmallVector<spirv_cross::Resource> *stageResources = nullptr;
            if (language == EShLangVertex) {
                stageResources = &resources.stage_outputs;
            }
            else if (language == EShLangFragment) {
                stageResources = &resources.stage_inputs;
            }
            if (stageResources && !options.interfaceLocations.empty()) {
                for (const auto &resource : *stageResources) {
                    auto loc = options.interfaceLocations.find(resource.name);
                    if (loc != options.interfaceLocations.end()) {
                        compiler.set_decoration(
                            resource.id, spv::DecorationLocation, static_cast<uint32_t>(loc->second));
                    }
                }
            }
            restoreIfNeeded(host, language, attributes, compiler);
            compiler.add_header_line(metalShaderPreamble());
            std::ostringstream os;
            os << compiler.compile();
            return os.str();
        };
        result.vertexSource = compileStage(
            static_cast<ShaderStageLanguage>(EShLangVertex), *request.vertexSPIRV, request.vertexSamplers);
        result.fragmentSource = compileStage(
            static_cast<ShaderStageLanguage>(EShLangFragment), *request.fragmentSPIRV, request.fragmentSamplers);
        result.succeeded = true;
    } catch (const spirv_cross::CompilerError &e) {
        errors.insert(e.what());
    } catch (const std::exception &e) {
        errors.insert(e.what());
    }
    return result.succeeded;
}

bool
translateVulkan(const TranslateRequest &request, const VulkanOptions &options, const HostServices &host,
    TranslateResult &result, ErrorSink &errors)
{
    result = TranslateResult();
    result.api = GraphicsAPI::Vulkan;
    if (!hasBothStages(request)) {
        return false;
    }
    try {
        auto processStage = [&](const SPIRVWords &words, const SamplerBindingMap *samplers, SPIRVWords &outWords) {
            SPIRVWords optimized;
            optimizeStage(host, words, optimized, errors);
            outWords = std::move(optimized);
            if (options.applySamplerBindings) {
                if (!rewriteVulkanSamplerBindings(outWords, samplers, options.descriptorSet, errors)) {
                    return false;
                }
            }
            return !outWords.empty();
        };
        if (!processStage(*request.vertexSPIRV, request.vertexSamplers, result.vertexSPIRV) ||
            !processStage(*request.fragmentSPIRV, request.fragmentSamplers, result.fragmentSPIRV)) {
            return false;
        }
        result.succeeded = true;
    } catch (const spirv_cross::CompilerError &e) {
        errors.insert(e.what());
    } catch (const std::exception &e) {
        errors.insert(e.what());
    }
    return result.succeeded;
}

bool
translate(GraphicsAPI api, const TranslateRequest &request, const BackendOptions &options, const HostServices &host,
    TranslateResult &result, ErrorSink &errors)
{
    switch (api) {
    case GraphicsAPI::OpenGL:
        return translateOpenGL(request, options.opengl, host, result, errors);
    case GraphicsAPI::OpenGLES:
        return translateOpenGLES(request, options.opengl, host, result, errors);
    case GraphicsAPI::DirectX:
        return translateDirectX(request, options.directx, host, result, errors);
    case GraphicsAPI::Metal:
        return translateMetal(request, options.metal, host, result, errors);
    case GraphicsAPI::Vulkan:
        return translateVulkan(request, options.vulkan, host, result, errors);
    default:
        errors.insert("unknown graphics API");
        return false;
    }
}

} /* namespace graphics */
} /* namespace fx9 */

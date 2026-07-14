/*
   Copyright (c) 2015-2023 hkrn All rights reserved

   This file is licensed under MIT license. for more details, see LICENSE.txt.
 */

#include "fx9/ShaderCrossTranslator.h"

#include <algorithm>
#include <sstream>

#include "glslang/Public/ShaderLang.h"
#include "spirv_cross/spirv_glsl.hpp"
#include "spirv_cross/spirv_hlsl.hpp"
#include "spirv_cross/spirv_msl.hpp"

namespace fx9 {
namespace translation {
namespace {

bool
hasBothStages(const CrossTranslateRequest &request)
{
    return request.vertexSPIRV && request.fragmentSPIRV && !request.vertexSPIRV->empty() &&
        !request.fragmentSPIRV->empty();
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

void
restoreIfNeeded(const CrossHostServices &host, ShaderStageLanguage language, const AttributeNameMap &attributes,
    spirv_cross::Compiler &compiler)
{
    if (host.restoreAttributes) {
        host.restoreAttributes(language, attributes, &compiler);
    }
}

void
optimizeStage(const CrossHostServices &host, const SPIRVWords &inWords, SPIRVWords &outWords, ErrorSink &errors)
{
    if (host.optimize) {
        host.optimize(inWords, outWords, errors);
    }
    else {
        outWords = inWords;
    }
}

void
saveStageAttributes(const CrossHostServices &host, const SPIRVWords &words, AttributeNameMap &attributes)
{
    if (host.saveAttributes) {
        host.saveAttributes(words, attributes);
    }
}

std::vector<spirv_cross::HLSLVertexAttributeRemap>
buildHLSLVertexInputRemaps()
{
    /* Match historical Compiler::HLSLPassShader contract: descending from 0x7ff. */
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
buildHLSLFragmentInputRemaps()
{
    std::vector<spirv_cross::HLSLVertexAttributeRemap> mapping;
    for (uint32_t i = 0; i < 16; i++) {
        std::ostringstream s;
        s << "TEXCOORD" << i;
        mapping.push_back(spirv_cross::HLSLVertexAttributeRemap { i, s.str() });
    }
    return mapping;
}

} /* namespace anonymous */

void
configureGLSLBackend(void *compilerGLSL, const GLSLBackendOptions &options)
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
configureHLSLBackend(void *compilerHLSL, const HLSLBackendOptions &options)
{
    auto &compiler = *static_cast<spirv_cross::CompilerHLSL *>(compilerHLSL);
    spirv_cross::CompilerHLSL::Options opts;
    opts.shader_model = options.shaderModel;
    opts.point_size_compat = true;
    opts.point_coord_compat = true;
    compiler.set_hlsl_options(opts);
}

void
configureMSLBackend(void *compilerMSL, const MSLBackendOptions &options)
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

const char *
metalShaderPreamble()
{
    return "#pragma clang diagnostic ignored \"-Wunused-variable\"\n"
           "#include <metal_math>\n"
           "using namespace metal;\n"
           "float length(float x) { return sqrt(x * x); }\n";
}

bool
translateToGLSL(const CrossTranslateRequest &request, const GLSLBackendOptions &options,
    const CrossHostServices &host, CrossTranslateResult &result, ErrorSink &errors)
{
    result = CrossTranslateResult();
    if (!hasBothStages(request)) {
        return false;
    }
    try {
        auto compileStage = [&](ShaderStageLanguage language, const SPIRVWords &words) {
            SPIRVWords optimized;
            AttributeNameMap attributes;
            saveStageAttributes(host, words, attributes);
            optimizeStage(host, words, optimized, errors);
            spirv_cross::CompilerGLSL compiler(optimized);
            configureGLSLBackend(&compiler, options);
            restoreIfNeeded(host, language, attributes, compiler);
            return compiler.compile();
        };
        result.vertexSource = compileStage(static_cast<ShaderStageLanguage>(EShLangVertex), *request.vertexSPIRV);
        result.fragmentSource = compileStage(static_cast<ShaderStageLanguage>(EShLangFragment), *request.fragmentSPIRV);
        result.succeeded = true;
    } catch (const spirv_cross::CompilerError &e) {
        errors.insert(e.what());
    } catch (const std::exception &e) {
        errors.insert(e.what());
    }
    return result.succeeded;
}

bool
translateToHLSL(const CrossTranslateRequest &request, const HLSLBackendOptions &options,
    const CrossHostServices &host, CrossTranslateResult &result, ErrorSink &errors)
{
    result = CrossTranslateResult();
    if (!hasBothStages(request)) {
        return false;
    }
    try {
        auto compileStage = [&](ShaderStageLanguage /* language */, const SPIRVWords &words,
                                const std::vector<spirv_cross::HLSLVertexAttributeRemap> &mapping,
                                const SamplerBindingMap *samplers) {
            SPIRVWords optimized;
            optimizeStage(host, words, optimized, errors);
            spirv_cross::CompilerHLSL compiler(optimized);
            configureHLSLBackend(&compiler, options);
            applySamplerBindingsHLSL(compiler, samplers);
            for (const auto &remap : mapping) {
                compiler.add_vertex_attribute_remap(remap);
            }
            std::ostringstream stream;
            stream << compiler.compile();
            return stream.str();
        };
        result.vertexSource =
            compileStage(static_cast<ShaderStageLanguage>(EShLangVertex), *request.vertexSPIRV, buildHLSLVertexInputRemaps(), request.vertexSamplers);
        result.fragmentSource = compileStage(
            static_cast<ShaderStageLanguage>(EShLangFragment), *request.fragmentSPIRV, buildHLSLFragmentInputRemaps(),
            request.fragmentSamplers);
        result.succeeded = true;
    } catch (const spirv_cross::CompilerError &e) {
        errors.insert(e.what());
    } catch (const std::exception &e) {
        errors.insert(e.what());
    }
    return result.succeeded;
}

bool
translateToMSL(const CrossTranslateRequest &request, const MSLBackendOptions &options,
    const CrossHostServices &host, CrossTranslateResult &result, ErrorSink &errors)
{
    result = CrossTranslateResult();
    if (!hasBothStages(request)) {
        return false;
    }
    try {
        auto compileStage = [&](ShaderStageLanguage language, const SPIRVWords &words, const SamplerBindingMap *samplers) {
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
            configureMSLBackend(&compiler, options);
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
                        compiler.set_decoration(resource.id, spv::DecorationLocation, static_cast<uint32_t>(loc->second));
                    }
                }
            }
            restoreIfNeeded(host, language, attributes, compiler);
            compiler.add_header_line(metalShaderPreamble());
            std::ostringstream os;
            os << compiler.compile();
            return os.str();
        };
        result.vertexSource = compileStage(static_cast<ShaderStageLanguage>(EShLangVertex), *request.vertexSPIRV, request.vertexSamplers);
        result.fragmentSource = compileStage(static_cast<ShaderStageLanguage>(EShLangFragment), *request.fragmentSPIRV, request.fragmentSamplers);
        result.succeeded = true;
    } catch (const spirv_cross::CompilerError &e) {
        errors.insert(e.what());
    } catch (const std::exception &e) {
        errors.insert(e.what());
    }
    return result.succeeded;
}

} /* namespace translation */
} /* namespace fx9 */

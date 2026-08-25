/*
   Copyright (c) 2015-2023 hkrn All rights reserved

   This file is licensed under MIT license. for more details, see LICENSE.txt.
 */

#include "fx9next/Translator.h"

#include "spirv_cross/spirv_glsl.hpp"
#include "spirv_cross/spirv_hlsl.hpp"
#include "spirv_cross/spirv_msl.hpp"
#include "spirv_cross/spirv_cross_error_handling.hpp"

#include <exception>

namespace fx9next {
namespace {

bool
crossCompile(const std::vector<uint32_t> &words, const TranslateOptions &options, bool fragment, std::string &source,
    std::string &error)
{
    if (words.empty()) {
        source.clear();
        return true;
    }
    try {
        if (options.language == kLanguageTypeHLSL) {
            spirv_cross::CompilerHLSL compiler(words);
            spirv_cross::CompilerHLSL::Options hlsl;
            hlsl.shader_model = 41;
            compiler.set_hlsl_options(hlsl);
            source = compiler.compile();
            return true;
        }
        if (options.language == kLanguageTypeMSL) {
            spirv_cross::CompilerMSL compiler(words);
            spirv_cross::CompilerMSL::Options msl;
            msl.msl_version = spirv_cross::CompilerMSL::Options::make_msl_version(2, 0);
            compiler.set_msl_options(msl);
            if (!options.metalEntry.empty()) {
                compiler.rename_entry_point("main", options.metalEntry,
                    fragment ? spv::ExecutionModelFragment : spv::ExecutionModelVertex);
            }
            try {
                const spirv_cross::ShaderResources res = compiler.get_shader_resources();
                for (const auto &ub : res.uniform_buffers) {
                    compiler.set_name(ub.id, options.metalUbo.empty() ? "nanoem_uniforms" : options.metalUbo);
                    auto type = compiler.get_type_from_variable(ub.id);
                    compiler.set_member_name(type.self, 0, fragment ? "ps_uniforms_vec4" : "vs_uniforms_vec4");
                }
            } catch (...) {}
            source = compiler.compile();
            if (source.empty()) {
                error = "MSL compiler returned empty source";
                return false;
            }
            return true;
        }
        if (options.language == kLanguageTypeSPIRV) {
            source.clear();
            return true;
        }
        spirv_cross::CompilerGLSL compiler(words);
        spirv_cross::CompilerGLSL::Options glsl;
        glsl.es = options.language == kLanguageTypeESSL;
        glsl.version = options.version > 0 ? static_cast<uint32_t>(options.version) : (glsl.es ? 300u : 330u);
        compiler.set_common_options(glsl);
        try {
            const spirv_cross::ShaderResources res = compiler.get_shader_resources();
            for (const auto &ub : res.uniform_buffers) {
                compiler.set_name(ub.id, fragment ? "ps_uniforms_vec4_buffer" : "vs_uniforms_vec4_buffer");
                auto type = compiler.get_type_from_variable(ub.id);
                compiler.set_member_name(type.self, 0, fragment ? "ps_uniforms_vec4" : "vs_uniforms_vec4");
            }
        } catch (...) {}
        source = compiler.compile();
        return true;
    }
    catch (const spirv_cross::CompilerError &ex) {
        error = ex.what();
        return false;
    }
    catch (const std::exception &ex) {
        error = ex.what();
        return false;
    }
    catch (...) {
        error = "unknown SPIRV-Cross error";
        return false;
    }
}

} /* namespace anonymous */

bool
translateSPIRV(const std::vector<uint32_t> &vertex, const std::vector<uint32_t> &fragment,
    const TranslateOptions &options, std::string &vertexSource, std::string &fragmentSource,
    std::vector<uint32_t> &vertexOut, std::vector<uint32_t> &fragmentOut, std::string &error)
{
    vertexOut = vertex;
    fragmentOut = fragment;
    error.clear();
    if (options.language == kLanguageTypeSPIRV) {
        vertexSource.clear();
        fragmentSource.clear();
        return true;
    }
    if (!crossCompile(vertex, options, false, vertexSource, error)) {
        return false;
    }
    if (!crossCompile(fragment, options, true, fragmentSource, error)) {
        return false;
    }
    return true;
}

} /* namespace fx9next */

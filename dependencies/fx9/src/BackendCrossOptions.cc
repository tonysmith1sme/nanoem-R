/*
   Copyright (c) 2015-2023 hkrn All rights reserved

   This file is licensed under MIT license. for more details, see LICENSE.txt.
 */

#include "fx9/BackendCrossOptions.h"

#include "spirv_cross/spirv_glsl.hpp"
#include "spirv_cross/spirv_hlsl.hpp"
#include "spirv_cross/spirv_msl.hpp"

namespace fx9 {
namespace translation {

void
applyGLSLCrossOptions(spirv_cross::CompilerGLSL &compiler, const GLSLCrossConfig &config)
{
    spirv_cross::CompilerGLSL::Options options;
    options.enable_420pack_extension = false;
    options.es = config.es;
    options.version = static_cast<uint32_t>(config.version);
    options.vertex.fixup_clipspace = true;
    options.fragment.default_float_precision = spirv_cross::CompilerGLSL::Options::Highp;
    options.fragment.default_int_precision = spirv_cross::CompilerGLSL::Options::Highp;
    compiler.set_common_options(options);
}

void
applyHLSLCrossOptions(spirv_cross::CompilerHLSL &compiler, const HLSLCrossConfig &config)
{
    spirv_cross::CompilerHLSL::Options options;
    options.shader_model = config.shaderModel;
    /* Keep point size / point coord optional semantics for SM4.x translation stability. */
    options.point_size_compat = true;
    options.point_coord_compat = true;
    compiler.set_hlsl_options(options);
}

void
applyMSLCrossOptions(spirv_cross::CompilerMSL &compiler, const MSLCrossConfig &config)
{
    spirv_cross::CompilerMSL::Options options;
    options.platform = spirv_cross::CompilerMSL::Options::macOS;
    options.set_msl_version(config.major, config.minor);
    /* Pad fragment outputs to avoid Metal driver undefined writes on partial MRT. */
    options.pad_fragment_output_components = true;
    /* Honor SPIR-V binding decorations for sampler/texture binding remaps. */
    options.enable_decoration_binding = true;
    /* Prefer native arrays on Intel Mac Metal to avoid pointer-to-array ABI issues. */
    options.force_native_arrays = true;
    compiler.set_msl_options(options);
}

const char *
metalShaderPreamble()
{
    return "#pragma clang diagnostic ignored \"-Wunused-variable\"\n"
           "#include <metal_math>\n"
           "using namespace metal;\n"
           "float length(float x) { return sqrt(x * x); }\n";
}

} /* namespace translation */
} /* namespace fx9 */

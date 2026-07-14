/*
   Copyright (c) 2015-2023 hkrn All rights reserved

   This file is licensed under MIT license. for more details, see LICENSE.txt.
 */

#pragma once
#ifndef FX9_BACKEND_CROSS_OPTIONS_H_
#define FX9_BACKEND_CROSS_OPTIONS_H_

#include <string>

/* Forward-declare SPIRV-Cross option structs so Compiler.cc can share one
 * deterministic configuration path without duplicating magic constants. */
namespace spirv_cross {
class CompilerGLSL;
class CompilerHLSL;
class CompilerMSL;
} /* namespace spirv_cross */

namespace fx9 {
namespace translation {

struct GLSLCrossConfig {
    bool es = false;
    int version = 330;
};

struct HLSLCrossConfig {
    int shaderModel = 41;
};

struct MSLCrossConfig {
    int major = 2;
    int minor = 0;
    std::string entryPoint = "fx9_metal_main";
};

/* Apply stable, documented SPIRV-Cross options used by all fx9 backends. */
void applyGLSLCrossOptions(spirv_cross::CompilerGLSL &compiler, const GLSLCrossConfig &config);
void applyHLSLCrossOptions(spirv_cross::CompilerHLSL &compiler, const HLSLCrossConfig &config);
void applyMSLCrossOptions(spirv_cross::CompilerMSL &compiler, const MSLCrossConfig &config);

/* Header preamble injected into every translated Metal shader. */
const char *metalShaderPreamble();

} /* namespace translation */
} /* namespace fx9 */

#endif /* FX9_BACKEND_CROSS_OPTIONS_H_ */

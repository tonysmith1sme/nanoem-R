/*
   Copyright (c) 2015-2023 hkrn All rights reserved

   This file is licensed under MIT license. for more details, see LICENSE.txt.
 */

#pragma once
#ifndef FX9_EFFECT_SOURCE_PIPELINE_H_
#define FX9_EFFECT_SOURCE_PIPELINE_H_

#include <string>

namespace fx9 {
namespace translation {

/**
 * Modular DX9 Effect source preparation pipeline.
 *
 * Owns every pre-parse rewrite that must run before Lemon / glslang:
 *   1. CompatibilityProfiles – path/profile-keyed ray-mmd & legacy fixes
 *   2. SourceNormalizer      – SJIS residues, string escapes
 *   3. LegacyEffectRules     – DefTech expansion
 *   4. MacroHygiene          – #define redefinition cleanup (always last)
 *
 * Shader body cross-compilation lives in ShaderCrossTranslator (SPIR-V ->
 * GLSL / HLSL SM4.1 / MSL). This module never touches SPIRV-Cross.
 */
std::string prepareEffectSource(const std::string &path, const std::string &source);

} /* namespace translation */
} /* namespace fx9 */

#endif /* FX9_EFFECT_SOURCE_PIPELINE_H_ */

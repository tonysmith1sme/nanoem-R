/*
   Copyright (c) 2015-2023 hkrn All rights reserved

   This file is licensed under MIT license. for more details, see LICENSE.txt.
 */

#pragma once
#ifndef FX9_EFFECT_TRANSLATOR_H_
#define FX9_EFFECT_TRANSLATOR_H_

#include <string>

namespace fx9 {
namespace translation {

/**
 * Structured pre-translation pipeline for MME DX9 Effect sources.
 *
 * Replaces the previous ad-hoc EffectSourcePatch regex layer with an ordered,
 * rule-based pipeline:
 *   1. SourceNormalizer   – encoding residues, string escapes, macro hygiene
 *   2. LegacyEffectRules  – DefTech expansion and other pre-SM4.0 constructs
 *   3. CompatibilityRules – profile-driven rewrites (e.g. ray-mmd on Metal/HLSL)
 *
 * Shader body translation still uses glslang (HLSL→SPIR-V) + SPIRV-Cross
 * (SPIR-V→MSL/HLSL/GLSL). This module only owns the source preparation stage
 * that must run before Lemon/glslang parse.
 */
std::string prepareEffectSource(const std::string &path, const std::string &source);

} /* namespace translation */
} /* namespace fx9 */

#endif /* FX9_EFFECT_TRANSLATOR_H_ */

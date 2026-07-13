/*
   Copyright (c) 2015-2023 hkrn All rights reserved

   This file is licensed under MIT license. for more details, see LICENSE.txt.
 */

#pragma once

#include <string>

namespace fx9 {
namespace effect {

/* Shared ray-mmd / legacy HLSL source rewrites used by both Compiler and Parser.
 * Keeping one implementation avoids divergent Metal/HLSL quality overrides and
 * prevents double-maintenance of the YCbCr / GBuffer / SHKernel workarounds. */
std::string patchRayMMDSource(const std::string &path, const std::string &source);
std::string patchLegacyEffectSource(const std::string &path, const std::string &source);

} /* namespace effect */
} /* namespace fx9 */

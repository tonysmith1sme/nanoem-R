/*
   Copyright (c) 2015-2023 hkrn All rights reserved

   This file is licensed under MIT license. for more details, see LICENSE.txt.
 */

#pragma once
#ifndef FX9NEXT_SPIRV_EMITTER_H_
#define FX9NEXT_SPIRV_EMITTER_H_

#include <stdint.h>
#include <string>
#include <vector>

#include "fx9next/AST.h"
#include "fx9next/ShaderIR.h"

namespace fx9next {

enum SpirvShaderStage { kStageVertex, kStageFragment };

bool emitFunctionSPIRV(const TranslationUnit &unit, const Function &fn, SpirvShaderStage stage,
    std::vector<uint32_t> &words, std::string &error);
bool emitShaderSPIRV(const TranslationUnit &unit, const ShaderModuleIR &shader, std::vector<uint32_t> &words,
    std::string &error);

} /* namespace fx9next */

#endif /* FX9NEXT_SPIRV_EMITTER_H_ */

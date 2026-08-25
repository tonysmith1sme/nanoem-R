/*
   Copyright (c) 2015-2023 hkrn All rights reserved

   This file is licensed under MIT license. for more details, see LICENSE.txt.
 */

#pragma once
#ifndef FX9NEXT_LOWERING_H_
#define FX9NEXT_LOWERING_H_

#include <string>

#include "fx9next/AST.h"
#include "fx9next/Diagnostics.h"
#include "fx9next/EffectIR.h"

namespace fx9next {

class Lowering {
public:
    bool lower(const TranslationUnit &unit, std::vector<ShaderModuleIR> &shaders, EffectModuleIR &effect,
        DiagnosticSink &diagnostics) const;

private:
    bool lowerScript(const std::string &text, std::vector<EffectScriptCommandIR> &commands,
        const std::string &entity, DiagnosticSink &diagnostics) const;
};

} /* namespace fx9next */

#endif /* FX9NEXT_LOWERING_H_ */

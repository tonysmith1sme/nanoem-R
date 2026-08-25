/*
   Copyright (c) 2015-2023 hkrn All rights reserved

   This file is licensed under MIT license. for more details, see LICENSE.txt.
 */

#pragma once
#ifndef FX9NEXT_EFFECT_IR_H_
#define FX9NEXT_EFFECT_IR_H_

#include <string>
#include <vector>

#include "fx9next/ShaderIR.h"

namespace fx9next {

enum EffectScriptCommandType {
    kEffectScriptRenderColorTarget,
    kEffectScriptRenderDepthStencilTarget,
    kEffectScriptClear,
    kEffectScriptPass,
    kEffectScriptDraw,
    kEffectScriptLoop
};

struct EffectScriptCommandIR {
    EffectScriptCommandType type;
    std::string name;
    std::string value;
    int index;
};

struct EffectResourceIR {
    std::string name;
    std::string type;
    std::string format;
    int mipLevels;
    bool shared;
};

struct EffectPassIR {
    std::string name;
    std::vector<EffectScriptCommandIR> script;
    std::vector<ShaderModuleIR> shaders;
    std::vector<std::pair<std::string, std::string> > renderStates;
};

struct EffectTechniqueIR {
    std::string name;
    std::vector<EffectScriptCommandIR> script;
    std::vector<EffectPassIR> passes;
};

struct EffectModuleIR {
    std::vector<EffectResourceIR> resources;
    std::vector<EffectTechniqueIR> techniques;

    std::string canonicalDump() const;
};

} /* namespace fx9next */

#endif /* FX9NEXT_EFFECT_IR_H_ */

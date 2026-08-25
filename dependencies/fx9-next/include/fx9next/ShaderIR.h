/*
   Copyright (c) 2015-2023 hkrn All rights reserved

   This file is licensed under MIT license. for more details, see LICENSE.txt.
 */

#pragma once
#ifndef FX9NEXT_SHADER_IR_H_
#define FX9NEXT_SHADER_IR_H_

#include <string>
#include <vector>

#include "fx9next/Type.h"

namespace fx9next {

enum ShaderStage {
    kShaderStageVertex,
    kShaderStagePixel
};

struct ShaderParameterIR {
    std::string name;
    Type type;
    std::string semantic;
    bool input;
    bool output;
};

struct ShaderFunctionIR {
    std::string name;
    Type returnType;
    std::vector<ShaderParameterIR> parameters;
};

struct ShaderModuleIR {
    ShaderStage stage;
    std::string entryPoint;
    std::vector<ShaderParameterIR> inputs;
    std::vector<ShaderParameterIR> outputs;
    std::vector<ShaderFunctionIR> functions;

    std::string canonicalDump() const;
};

} /* namespace fx9next */

#endif /* FX9NEXT_SHADER_IR_H_ */

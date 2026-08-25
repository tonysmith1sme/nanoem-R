/*
   Copyright (c) 2015-2023 hkrn All rights reserved

   This file is licensed under MIT license. for more details, see LICENSE.txt.
 */

#pragma once
#ifndef FX9NEXT_SHADER_IR_H_
#define FX9NEXT_SHADER_IR_H_

#include <memory>
#include <string>
#include <vector>

#include "fx9next/Type.h"

namespace fx9next {

enum ShaderStage {
    kShaderStageVertex,
    kShaderStagePixel
};

enum ShaderExpressionKind {
    kShaderExpressionLiteralFloat,
    kShaderExpressionLiteralInt,
    kShaderExpressionLiteralBool,
    kShaderExpressionLiteralString,
    kShaderExpressionIdentifier,
    kShaderExpressionUnary,
    kShaderExpressionBinary,
    kShaderExpressionTernary,
    kShaderExpressionCall,
    kShaderExpressionMember,
    kShaderExpressionIndex,
    kShaderExpressionConstruct,
    kShaderExpressionCast
};

enum ShaderStatementKind {
    kShaderStatementBlock,
    kShaderStatementExpression,
    kShaderStatementReturn,
    kShaderStatementIf,
    kShaderStatementFor,
    kShaderStatementWhile,
    kShaderStatementDoWhile,
    kShaderStatementDiscard,
    kShaderStatementVariable,
    kShaderStatementBreak,
    kShaderStatementContinue
};

struct ShaderExpressionIR {
    ShaderExpressionIR();
    ShaderExpressionIR(const ShaderExpressionIR &other);
    ShaderExpressionIR &operator=(const ShaderExpressionIR &other);

    ShaderExpressionKind kind;
    Type type;
    std::string name;
    std::string operation;
    double floatValue;
    int intValue;
    bool boolValue;
    std::vector<std::unique_ptr<ShaderExpressionIR> > children;
};

struct ShaderStatementIR {
    ShaderStatementIR();
    ShaderStatementIR(const ShaderStatementIR &other);
    ShaderStatementIR &operator=(const ShaderStatementIR &other);

    ShaderStatementKind kind;
    Type variableType;
    std::string name;
    std::string semantic;
    std::unique_ptr<ShaderExpressionIR> expression;
    std::unique_ptr<ShaderExpressionIR> condition;
    std::unique_ptr<ShaderExpressionIR> iteration;
    std::unique_ptr<ShaderStatementIR> thenStatement;
    std::unique_ptr<ShaderStatementIR> elseStatement;
    std::vector<std::unique_ptr<ShaderStatementIR> > children;
};

struct ShaderParameterIR {
    std::string name;
    Type type;
    std::string semantic;
    bool input;
    bool output;
};

struct ShaderFunctionIR {
    ShaderFunctionIR();
    ShaderFunctionIR(const ShaderFunctionIR &other);
    ShaderFunctionIR &operator=(const ShaderFunctionIR &other);

    std::string name;
    Type returnType;
    std::string returnSemantic;
    std::vector<ShaderParameterIR> parameters;
    std::unique_ptr<ShaderStatementIR> body;
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

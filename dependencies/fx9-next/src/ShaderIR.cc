/*
   Copyright (c) 2015-2023 hkrn All rights reserved

   This file is licensed under MIT license. for more details, see LICENSE.txt.
 */

#include "fx9next/ShaderIR.h"

#include <sstream>

namespace fx9next {
namespace {

std::unique_ptr<ShaderExpressionIR>
cloneExpression(const std::unique_ptr<ShaderExpressionIR> &expression)
{
    return expression ? std::unique_ptr<ShaderExpressionIR>(new ShaderExpressionIR(*expression)) :
                        std::unique_ptr<ShaderExpressionIR>();
}

std::unique_ptr<ShaderStatementIR>
cloneStatement(const std::unique_ptr<ShaderStatementIR> &statement)
{
    return statement ? std::unique_ptr<ShaderStatementIR>(new ShaderStatementIR(*statement)) :
                       std::unique_ptr<ShaderStatementIR>();
}

const char *
statementName(ShaderStatementKind kind)
{
    static const char *const kNames[] = {
        "block", "expr", "return", "if", "for", "while", "do-while", "discard", "var", "break", "continue"
    };
    return kNames[kind];
}

void
writeExpression(std::ostringstream &stream, const ShaderExpressionIR *expression)
{
    if (!expression) {
        return;
    }
    stream << expression->type.toString() << " ";
    switch (expression->kind) {
    case kShaderExpressionLiteralFloat:
        stream << expression->floatValue;
        break;
    case kShaderExpressionLiteralInt:
        stream << expression->intValue;
        break;
    case kShaderExpressionLiteralBool:
        stream << (expression->boolValue ? "true" : "false");
        break;
    default:
        stream << expression->name << expression->operation;
        break;
    }
    if (!expression->children.empty()) {
        stream << "(";
        for (size_t i = 0; i < expression->children.size(); i++) {
            if (i > 0) {
                stream << ",";
            }
            writeExpression(stream, expression->children[i].get());
        }
        stream << ")";
    }
}

void
writeStatement(std::ostringstream &stream, const ShaderStatementIR *statement, int depth)
{
    if (!statement) {
        return;
    }
    stream << std::string(static_cast<size_t>(depth) * 2, ' ') << statementName(statement->kind);
    if (!statement->name.empty()) {
        stream << " " << statement->variableType.toString() << " " << statement->name;
    }
    if (statement->expression) {
        stream << " ";
        writeExpression(stream, statement->expression.get());
    }
    if (statement->condition) {
        stream << " condition=";
        writeExpression(stream, statement->condition.get());
    }
    if (statement->iteration) {
        stream << " iteration=";
        writeExpression(stream, statement->iteration.get());
    }
    stream << "\n";
    for (std::vector<std::unique_ptr<ShaderStatementIR> >::const_iterator it = statement->children.begin(); it != statement->children.end(); ++it) {
        writeStatement(stream, it->get(), depth + 1);
    }
    writeStatement(stream, statement->thenStatement.get(), depth + 1);
    writeStatement(stream, statement->elseStatement.get(), depth + 1);
}

} /* namespace anonymous */

ShaderExpressionIR::ShaderExpressionIR()
    : kind(kShaderExpressionLiteralFloat)
    , floatValue(0)
    , intValue(0)
    , boolValue(false)
{
}

ShaderExpressionIR::ShaderExpressionIR(const ShaderExpressionIR &other)
    : kind(other.kind)
    , type(other.type)
    , name(other.name)
    , operation(other.operation)
    , floatValue(other.floatValue)
    , intValue(other.intValue)
    , boolValue(other.boolValue)
{
    for (std::vector<std::unique_ptr<ShaderExpressionIR> >::const_iterator it = other.children.begin();
         it != other.children.end(); ++it) {
        children.push_back(cloneExpression(*it));
    }
}

ShaderExpressionIR &
ShaderExpressionIR::operator=(const ShaderExpressionIR &other)
{
    if (this != &other) {
        ShaderExpressionIR copy(other);
        std::swap(kind, copy.kind);
        type = copy.type;
        name.swap(copy.name);
        operation.swap(copy.operation);
        std::swap(floatValue, copy.floatValue);
        std::swap(intValue, copy.intValue);
        std::swap(boolValue, copy.boolValue);
        children.swap(copy.children);
    }
    return *this;
}

ShaderStatementIR::ShaderStatementIR()
    : kind(kShaderStatementBlock)
{
}

ShaderStatementIR::ShaderStatementIR(const ShaderStatementIR &other)
    : kind(other.kind)
    , variableType(other.variableType)
    , name(other.name)
    , semantic(other.semantic)
    , expression(cloneExpression(other.expression))
    , condition(cloneExpression(other.condition))
    , iteration(cloneExpression(other.iteration))
    , thenStatement(cloneStatement(other.thenStatement))
    , elseStatement(cloneStatement(other.elseStatement))
{
    for (std::vector<std::unique_ptr<ShaderStatementIR> >::const_iterator it = other.children.begin();
         it != other.children.end(); ++it) {
        children.push_back(cloneStatement(*it));
    }
}

ShaderStatementIR &
ShaderStatementIR::operator=(const ShaderStatementIR &other)
{
    if (this != &other) {
        ShaderStatementIR copy(other);
        std::swap(kind, copy.kind);
        variableType = copy.variableType;
        name.swap(copy.name);
        semantic.swap(copy.semantic);
        expression.swap(copy.expression);
        condition.swap(copy.condition);
        iteration.swap(copy.iteration);
        thenStatement.swap(copy.thenStatement);
        elseStatement.swap(copy.elseStatement);
        children.swap(copy.children);
    }
    return *this;
}

ShaderFunctionIR::ShaderFunctionIR()
{
}

ShaderFunctionIR::ShaderFunctionIR(const ShaderFunctionIR &other)
    : name(other.name)
    , returnType(other.returnType)
    , returnSemantic(other.returnSemantic)
    , parameters(other.parameters)
    , body(cloneStatement(other.body))
{
}

ShaderFunctionIR &
ShaderFunctionIR::operator=(const ShaderFunctionIR &other)
{
    if (this != &other) {
        ShaderFunctionIR copy(other);
        name.swap(copy.name);
        returnType = copy.returnType;
        returnSemantic.swap(copy.returnSemantic);
        parameters.swap(copy.parameters);
        body.swap(copy.body);
    }
    return *this;
}

std::string
ShaderModuleIR::canonicalDump() const
{
    std::ostringstream stream;
    stream << (stage == kShaderStageVertex ? "vertex" : "pixel") << " " << entryPoint << "\n";
    for (std::vector<ShaderParameterIR>::const_iterator it = inputs.begin(); it != inputs.end(); ++it) {
        stream << "in " << it->type.toString() << " " << it->name << " : " << it->semantic << "\n";
    }
    for (std::vector<ShaderStructIR>::const_iterator it = structs.begin(); it != structs.end(); ++it) {
        stream << "struct " << it->name;
        for (std::vector<std::pair<std::string, Type> >::const_iterator member = it->members.begin();
             member != it->members.end(); ++member) {
            stream << " " << member->second.toString() << " " << member->first;
            if (!member->second.name.empty()) {
                stream << " : " << member->second.name;
            }
        }
        stream << "\n";
    }
    for (std::vector<ShaderParameterIR>::const_iterator it = outputs.begin(); it != outputs.end(); ++it) {
        stream << "out " << it->type.toString() << " " << it->name << " : " << it->semantic << "\n";
    }
    for (std::vector<ShaderFunctionIR>::const_iterator it = functions.begin(); it != functions.end(); ++it) {
        stream << "fn " << it->returnType.toString() << " " << it->name << "(";
        for (size_t i = 0; i < it->parameters.size(); i++) {
            if (i > 0) {
                stream << ",";
            }
            stream << it->parameters[i].type.toString() << " " << it->parameters[i].name;
        }
        stream << ")\n";
        writeStatement(stream, it->body.get(), 1);
    }
    return stream.str();
}

} /* namespace fx9next */

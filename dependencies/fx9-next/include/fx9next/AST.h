/*
   Copyright (c) 2015-2023 hkrn All rights reserved

   This file is licensed under MIT license. for more details, see LICENSE.txt.
 */

#pragma once
#ifndef FX9NEXT_AST_H_
#define FX9NEXT_AST_H_

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "fx9next/Type.h"

namespace fx9next {

enum ExprKind {
    kExprLiteralFloat,
    kExprLiteralInt,
    kExprLiteralBool,
    kExprLiteralString,
    kExprIdent,
    kExprUnary,
    kExprBinary,
    kExprTernary,
    kExprCall,
    kExprMember,
    kExprIndex,
    kExprConstruct,
    kExprCast
};

enum StmtKind {
    kStmtBlock,
    kStmtExpr,
    kStmtReturn,
    kStmtIf,
    kStmtFor,
    kStmtWhile,
    kStmtDoWhile,
    kStmtDiscard,
    kStmtVar,
    kStmtBreak,
    kStmtContinue
};

struct Expr {
    ExprKind kind;
    Type type;
    std::string name;
    std::string op;
    double floatValue;
    int intValue;
    bool boolValue;
    std::vector<std::unique_ptr<Expr> > kids;

    static std::unique_ptr<Expr> make(ExprKind kind);
};

struct Stmt {
    StmtKind kind;
    Type varType;
    std::string name;
    std::string semantic;
    std::unique_ptr<Expr> expr;
    std::unique_ptr<Expr> expr2;
    std::unique_ptr<Expr> expr3;
    std::unique_ptr<Stmt> thenStmt;
    std::unique_ptr<Stmt> elseStmt;
    std::vector<std::unique_ptr<Stmt> > kids;
};

struct Parameter {
    Type type;
    std::string name;
    std::string semantic;
    std::string registerName;
};

struct Annotation {
    std::string name;
    enum ValueKind { kAnnBool, kAnnInt, kAnnFloat, kAnnString } kind;
    bool bval;
    int ival;
    float fval;
    std::string sval;
};

struct Function {
    std::string name;
    Type returnType;
    std::string returnSemantic;
    std::vector<Parameter> params;
    std::unique_ptr<Stmt> body;
};

struct SamplerStateItem {
    std::string key;
    std::string value;
};

struct Variable {
    std::string name;
    Type type;
    std::string semantic;
    std::string registerName;
    std::string textureName;
    std::vector<Annotation> annotations;
    std::vector<SamplerStateItem> samplerStates;
    std::unique_ptr<Expr> initializer;
};

struct PassState {
    std::string name;
    std::string value;
    std::string compileProfile;
    std::string compileEntry;
};

struct Pass {
    std::string name;
    std::string vsEntry;
    std::string psEntry;
    std::vector<Annotation> annotations;
    std::vector<PassState> states;
};

struct Technique {
    std::string name;
    std::vector<Annotation> annotations;
    std::vector<Pass> passes;
};

struct TranslationUnit {
    std::vector<Variable> variables;
    std::vector<Function> functions;
    std::vector<Technique> techniques;
    std::vector<std::string> includes;
};

} /* namespace fx9next */

#endif /* FX9NEXT_AST_H_ */

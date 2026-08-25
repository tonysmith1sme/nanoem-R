/*
   Copyright (c) 2015-2023 hkrn All rights reserved

   This file is licensed under MIT license. for more details, see LICENSE.txt.
 */

#pragma once
#ifndef FX9NEXT_PARSER_H_
#define FX9NEXT_PARSER_H_

#include <string>
#include <unordered_map>

#include "fx9next/AST.h"
#include "fx9next/Lexer.h"

namespace fx9next {

class Parser {
public:
    bool parse(const std::string &source, const std::string &filename, TranslationUnit &unit, std::string &error);

private:
    bool parseDecl(TranslationUnit &unit);
    bool parseTechnique(TranslationUnit &unit);
    bool parsePass(Technique &technique);
    bool parseStruct();
    bool parseFunctionOrVar(const Type &type, const std::string &name);
    std::unique_ptr<Stmt> parseStmt();
    std::unique_ptr<Stmt> parseBlock();
    std::unique_ptr<Expr> parseExpr();
    std::unique_ptr<Expr> parseAssign();
    std::unique_ptr<Expr> parseTernary();
    std::unique_ptr<Expr> parseBinary(int minPrec);
    std::unique_ptr<Expr> parseUnary();
    std::unique_ptr<Expr> parsePostfix();
    std::unique_ptr<Expr> parsePrimary();
    bool parseType(Type &type);
    bool parseAnnotations(std::vector<Annotation> &annotations);
    bool parseSemantic(std::string &semantic);
    bool parseRegister(std::string &reg);
    bool parseVarTail(Type type, const std::string &firstName, std::unique_ptr<Stmt> &stmt);
    void skipAttributes();
    void errorAt(const std::string &message);
    int binaryPrec(const std::string &op) const;
    bool isTypeName(const std::string &name) const;
    bool isQualifier(const std::string &name) const;
    bool looksLikeDeclaration() const;

    Lexer m_lexer;
    TranslationUnit *m_unit;
    std::string m_error;
    std::unordered_map<std::string, Type> m_userTypes;
};

} /* namespace fx9next */

#endif /* FX9NEXT_PARSER_H_ */

/*
   Copyright (c) 2015-2023 hkrn All rights reserved

   This file is licensed under MIT license. for more details, see LICENSE.txt.
 */

#include "fx9next/Parser.h"

#include <cctype>

namespace fx9next {

void
Parser::errorAt(const std::string &message)
{
    if (m_error.empty()) {
        m_error = m_lexer.filename() + ":" + std::to_string(m_lexer.peek().line) + ": " + message;
    }
}

void
Parser::skipAttributes()
{
    while (m_lexer.accept("[")) {
        int depth = 1;
        while (depth > 0 && m_lexer.peek().kind != kTokEof) {
            if (m_lexer.accept("[")) {
                depth++;
            }
            else if (m_lexer.accept("]")) {
                depth--;
            }
            else {
                m_lexer.next();
            }
        }
    }
}

bool
Parser::parseType(Type &type)
{
    while (m_lexer.accept("const") || m_lexer.accept("static") || m_lexer.accept("uniform") || m_lexer.accept("shared") ||
        m_lexer.accept("row_major") || m_lexer.accept("column_major") || m_lexer.accept("in") || m_lexer.accept("out") ||
        m_lexer.accept("inout") || m_lexer.accept("inline") || m_lexer.accept("precise") || m_lexer.accept("volatile")) {
    }
    Token token = m_lexer.peek();
    if (token.kind != kTokKeyword && token.kind != kTokIdent) {
        return false;
    }
    if (!Type::parseBuiltin(token.text, type)) {
        if (token.text == "struct") {
            return false;
        }
        auto it = m_userTypes.find(token.text);
        if (it != m_userTypes.end()) {
            type = it->second;
        }
        else {
            type.kind = kTypeStruct;
            type.name = token.text;
        }
    }
    m_lexer.next();
    return true;
}

bool
Parser::isTypeName(const std::string &name) const
{
    Type dummy;
    if (Type::parseBuiltin(name, dummy)) {
        return true;
    }
    return m_userTypes.find(name) != m_userTypes.end();
}

bool
Parser::isQualifier(const std::string &name) const
{
    return name == "const" || name == "static" || name == "uniform" || name == "shared" || name == "inline" ||
        name == "precise" || name == "volatile" || name == "in" || name == "out" || name == "inout" ||
        name == "row_major" || name == "column_major";
}

bool
Parser::looksLikeDeclaration() const
{
    size_t i = 0;
    while (isQualifier(m_lexer.peek(i).text)) {
        i++;
    }
    return isTypeName(m_lexer.peek(i).text) &&
        (m_lexer.peek(i + 1).kind == kTokIdent || m_lexer.peek(i + 1).kind == kTokKeyword);
}

bool
Parser::parseSemantic(std::string &semantic)
{
    if (m_lexer.peek().text != ":") {
        return true;
    }
    if (m_lexer.peek(1).text == "register") {
        return true;
    }
    m_lexer.next();
    Token token = m_lexer.next();
    if (token.kind != kTokIdent && token.kind != kTokKeyword) {
        errorAt("expected semantic");
        return false;
    }
    semantic = token.text;
    if (m_lexer.accept("[")) {
        Token index = m_lexer.next();
        semantic += index.text;
        if (!m_lexer.accept("]")) {
            errorAt("expected ]");
            return false;
        }
    }
    return true;
}

bool
Parser::parseRegister(std::string &reg)
{
    if (m_lexer.peek().text == ":" && m_lexer.peek(1).text == "register") {
        m_lexer.next();
    }
    if (!m_lexer.accept("register")) {
        return true;
    }
    if (!m_lexer.accept("(")) {
        errorAt("expected ( after register");
        return false;
    }
    Token token = m_lexer.next();
    reg = token.text;
    if (m_lexer.accept("[")) {
        Token index = m_lexer.next();
        reg += index.text;
        m_lexer.accept("]");
    }
    if (!m_lexer.accept(")")) {
        errorAt("expected ) after register");
        return false;
    }
    return true;
}

bool
Parser::parseAnnotations(std::vector<Annotation> &annotations)
{
    if (!m_lexer.accept("<")) {
        return true;
    }
    while (!m_lexer.accept(">") && m_lexer.peek().kind != kTokEof) {
        Type dummy;
        parseType(dummy);
        Token name = m_lexer.next();
        if (!m_lexer.accept("=")) {
            errorAt("expected = in annotation");
            return false;
        }
        Annotation ann;
        ann.name = name.text;
        Token value = m_lexer.peek();
        if (value.kind == kTokString) {
            m_lexer.next();
            ann.kind = Annotation::kAnnString;
            ann.sval = value.text;
            while (m_lexer.peek().kind == kTokString) {
                ann.sval += m_lexer.next().text;
            }
        }
        else if (value.text == "true" || value.text == "false") {
            m_lexer.next();
            ann.kind = Annotation::kAnnBool;
            ann.bval = value.text == "true";
        }
        else if (value.kind == kTokNumber) {
            m_lexer.next();
            if (value.isFloat) {
                ann.kind = Annotation::kAnnFloat;
                ann.fval = static_cast<float>(value.number);
            }
            else {
                ann.kind = Annotation::kAnnInt;
                ann.ival = static_cast<int>(value.number);
            }
        }
        else {
            std::unique_ptr<Expr> expr = parseExpr();
            ann.kind = Annotation::kAnnInt;
            ann.ival = 0;
            (void) expr;
        }
        annotations.push_back(ann);
        m_lexer.accept(";");
    }
    return m_error.empty();
}

std::unique_ptr<Expr>
Parser::parsePrimary()
{
    Token token = m_lexer.peek();
    if (token.text == "true" || token.text == "false") {
        m_lexer.next();
        std::unique_ptr<Expr> expr = Expr::make(kExprLiteralBool);
        expr->boolValue = token.text == "true";
        expr->type = Type::boolType();
        return expr;
    }
    if (token.kind == kTokNumber) {
        m_lexer.next();
        if (token.isFloat) {
            std::unique_ptr<Expr> expr = Expr::make(kExprLiteralFloat);
            expr->floatValue = token.number;
            expr->type = Type::floatType();
            return expr;
        }
        std::unique_ptr<Expr> expr = Expr::make(kExprLiteralInt);
        expr->intValue = static_cast<int>(token.number);
        expr->floatValue = token.number;
        expr->type = Type::intType();
        return expr;
    }
    if (token.kind == kTokString) {
        m_lexer.next();
        std::unique_ptr<Expr> expr = Expr::make(kExprLiteralString);
        expr->name = token.text;
        expr->type = Type::stringType();
        return expr;
    }
    Type constructed;
    if (Type::parseBuiltin(token.text, constructed) &&
        (constructed.kind == kTypeVector || constructed.kind == kTypeMatrix || constructed.kind == kTypeFloat ||
            constructed.kind == kTypeInt || constructed.kind == kTypeBool)) {
        m_lexer.next();
        if (m_lexer.accept("(")) {
            std::unique_ptr<Expr> expr = Expr::make(kExprConstruct);
            expr->type = constructed;
            expr->name = token.text;
            if (!m_lexer.accept(")")) {
                do {
                    expr->kids.push_back(parseAssign());
                } while (m_lexer.accept(","));
                if (!m_lexer.accept(")")) {
                    errorAt("expected )");
                }
            }
            return expr;
        }
        std::unique_ptr<Expr> expr = Expr::make(kExprIdent);
        expr->name = token.text;
        return expr;
    }
    if (token.kind == kTokIdent || token.kind == kTokKeyword) {
        m_lexer.next();
        std::unique_ptr<Expr> expr = Expr::make(kExprIdent);
        expr->name = token.text;
        return expr;
    }
    if (m_lexer.accept("{")) {
        std::unique_ptr<Expr> expr = Expr::make(kExprConstruct);
        expr->type = Type::vectorType(kTypeFloat, 4);
        if (!m_lexer.accept("}")) {
            do {
                if (m_lexer.peek().text == "}") {
                    break;
                }
                expr->kids.push_back(parseAssign());
            } while (m_lexer.accept(","));
            if (!m_lexer.accept("}")) {
                errorAt("expected }");
            }
        }
        return expr;
    }
    if (m_lexer.accept("(")) {
        Type castType;
        Token next = m_lexer.peek();
        if ((next.kind == kTokKeyword || next.kind == kTokIdent) && isTypeName(next.text) &&
            (m_lexer.peek(1).text == ")" || m_lexer.peek(1).text == "[")) {
            if (!Type::parseBuiltin(next.text, castType)) {
                auto it = m_userTypes.find(next.text);
                if (it != m_userTypes.end()) {
                    castType = it->second;
                }
            }
            m_lexer.next();
            if (m_lexer.accept("[")) {
                m_lexer.next();
                if (!m_lexer.accept("]")) {
                    errorAt("expected ]");
                }
            }
            if (!m_lexer.accept(")")) {
                errorAt("expected )");
            }
            std::unique_ptr<Expr> expr = Expr::make(kExprCast);
            expr->type = castType;
            expr->kids.push_back(parseUnary());
            return expr;
        }
        std::unique_ptr<Expr> expr = parseAssign();
        if (!m_lexer.accept(")")) {
            errorAt("expected )");
        }
        return expr;
    }
    errorAt("expected expression");
    return Expr::make(kExprLiteralInt);
}

std::unique_ptr<Expr>
Parser::parsePostfix()
{
    std::unique_ptr<Expr> expr = parsePrimary();
    for (;;) {
        if (m_lexer.accept("(")) {
            std::unique_ptr<Expr> call = Expr::make(kExprCall);
            call->name = expr->name;
            call->kids.push_back(std::move(expr));
            if (!m_lexer.accept(")")) {
                do {
                    call->kids.push_back(parseAssign());
                } while (m_lexer.accept(","));
                if (!m_lexer.accept(")")) {
                    errorAt("expected )");
                }
            }
            expr = std::move(call);
            continue;
        }
        if (m_lexer.accept(".")) {
            Token member = m_lexer.next();
            std::unique_ptr<Expr> dot = Expr::make(kExprMember);
            dot->name = member.text;
            dot->kids.push_back(std::move(expr));
            expr = std::move(dot);
            continue;
        }
        if (m_lexer.accept("[")) {
            std::unique_ptr<Expr> index = Expr::make(kExprIndex);
            index->kids.push_back(std::move(expr));
            index->kids.push_back(parseAssign());
            if (!m_lexer.accept("]")) {
                errorAt("expected ]");
            }
            expr = std::move(index);
            continue;
        }
        if (m_lexer.peek().text == "++" || m_lexer.peek().text == "--") {
            std::unique_ptr<Expr> inc = Expr::make(kExprUnary);
            inc->op = m_lexer.next().text;
            inc->kids.push_back(std::move(expr));
            expr = std::move(inc);
            continue;
        }
        break;
    }
    return expr;
}

std::unique_ptr<Expr>
Parser::parseUnary()
{
    Token token = m_lexer.peek();
    if (token.text == "+" || token.text == "-" || token.text == "!" || token.text == "~" || token.text == "++" ||
        token.text == "--") {
        m_lexer.next();
        std::unique_ptr<Expr> expr = Expr::make(kExprUnary);
        expr->op = token.text;
        expr->kids.push_back(parseUnary());
        return expr;
    }
    return parsePostfix();
}

int
Parser::binaryPrec(const std::string &op) const
{
    if (op == "*" || op == "/" || op == "%") {
        return 50;
    }
    if (op == "+" || op == "-") {
        return 40;
    }
    if (op == "<<" || op == ">>") {
        return 35;
    }
    if (op == "<" || op == ">" || op == "<=" || op == ">=") {
        return 30;
    }
    if (op == "==" || op == "!=") {
        return 25;
    }
    if (op == "&") {
        return 22;
    }
    if (op == "^") {
        return 21;
    }
    if (op == "|") {
        return 20;
    }
    if (op == "&&") {
        return 15;
    }
    if (op == "||") {
        return 10;
    }
    return -1;
}

std::unique_ptr<Expr>
Parser::parseBinary(int minPrec)
{
    std::unique_ptr<Expr> left = parseUnary();
    for (;;) {
        int prec = binaryPrec(m_lexer.peek().text);
        if (prec < minPrec) {
            break;
        }
        std::string op = m_lexer.next().text;
        std::unique_ptr<Expr> right = parseBinary(prec + 1);
        std::unique_ptr<Expr> bin = Expr::make(kExprBinary);
        bin->op = op;
        bin->kids.push_back(std::move(left));
        bin->kids.push_back(std::move(right));
        left = std::move(bin);
    }
    return left;
}

std::unique_ptr<Expr>
Parser::parseTernary()
{
    std::unique_ptr<Expr> cond = parseBinary(0);
    if (m_lexer.accept("?")) {
        std::unique_ptr<Expr> expr = Expr::make(kExprTernary);
        expr->kids.push_back(std::move(cond));
        expr->kids.push_back(parseAssign());
        if (!m_lexer.accept(":")) {
            errorAt("expected :");
        }
        expr->kids.push_back(parseAssign());
        return expr;
    }
    return cond;
}

std::unique_ptr<Expr>
Parser::parseAssign()
{
    std::unique_ptr<Expr> left = parseTernary();
    Token token = m_lexer.peek();
    if (token.text == "=" || token.text == "+=" || token.text == "-=" || token.text == "*=" || token.text == "/=" ||
        token.text == "%=") {
        m_lexer.next();
        std::unique_ptr<Expr> expr = Expr::make(kExprBinary);
        expr->op = token.text;
        expr->kids.push_back(std::move(left));
        expr->kids.push_back(parseAssign());
        return expr;
    }
    return left;
}

std::unique_ptr<Expr>
Parser::parseExpr()
{
    return parseAssign();
}

std::unique_ptr<Stmt>
Parser::parseBlock()
{
    std::unique_ptr<Stmt> block(new Stmt());
    block->kind = kStmtBlock;
    if (!m_lexer.accept("{")) {
        errorAt("expected {");
        return block;
    }
    while (!m_lexer.accept("}") && m_lexer.peek().kind != kTokEof) {
        block->kids.push_back(parseStmt());
        if (!m_error.empty()) {
            break;
        }
    }
    return block;
}

std::unique_ptr<Stmt>
Parser::parseStmt()
{
    skipAttributes();
    Token token = m_lexer.peek();
    if (token.text == "{") {
        return parseBlock();
    }
    std::unique_ptr<Stmt> stmt(new Stmt());
    if (token.text == "return") {
        m_lexer.next();
        stmt->kind = kStmtReturn;
        if (!m_lexer.accept(";")) {
            stmt->expr = parseExpr();
            if (!m_lexer.accept(";")) {
                errorAt("expected ;");
            }
        }
        return stmt;
    }
    if (token.text == "discard") {
        m_lexer.next();
        stmt->kind = kStmtDiscard;
        m_lexer.accept(";");
        return stmt;
    }
    if (token.text == "break") {
        m_lexer.next();
        stmt->kind = kStmtBreak;
        m_lexer.accept(";");
        return stmt;
    }
    if (token.text == "continue") {
        m_lexer.next();
        stmt->kind = kStmtContinue;
        m_lexer.accept(";");
        return stmt;
    }
    if (token.text == "if") {
        m_lexer.next();
        stmt->kind = kStmtIf;
        if (!m_lexer.accept("(")) {
            errorAt("expected (");
        }
        stmt->expr = parseExpr();
        if (!m_lexer.accept(")")) {
            errorAt("expected )");
        }
        stmt->thenStmt = parseStmt();
        if (m_lexer.accept("else")) {
            stmt->elseStmt = parseStmt();
        }
        return stmt;
    }
    if (token.text == "for") {
        m_lexer.next();
        stmt->kind = kStmtFor;
        if (!m_lexer.accept("(")) {
            errorAt("expected (");
        }
        if (!m_lexer.accept(";")) {
            Type t;
            if (parseType(t) && (m_lexer.peek().kind == kTokIdent)) {
                stmt->name = m_lexer.next().text;
                stmt->varType = t;
                if (m_lexer.accept("=")) {
                    stmt->expr = parseExpr();
                }
            }
            else {
                stmt->expr = parseExpr();
            }
            if (!m_lexer.accept(";")) {
                errorAt("expected ;");
            }
        }
        if (!m_lexer.accept(";")) {
            stmt->expr2 = parseExpr();
            if (!m_lexer.accept(";")) {
                errorAt("expected ;");
            }
        }
        if (!m_lexer.accept(")")) {
            stmt->expr3 = parseExpr();
            if (!m_lexer.accept(")")) {
                errorAt("expected )");
            }
        }
        stmt->thenStmt = parseStmt();
        return stmt;
    }
    if (token.text == "while") {
        m_lexer.next();
        stmt->kind = kStmtWhile;
        if (!m_lexer.accept("(")) {
            errorAt("expected (");
        }
        stmt->expr = parseExpr();
        if (!m_lexer.accept(")")) {
            errorAt("expected )");
        }
        stmt->thenStmt = parseStmt();
        return stmt;
    }
    if (token.text == "do") {
        m_lexer.next();
        stmt->kind = kStmtDoWhile;
        stmt->thenStmt = parseStmt();
        if (!m_lexer.accept("while") || !m_lexer.accept("(")) {
            errorAt("expected while (");
        }
        stmt->expr = parseExpr();
        if (!m_lexer.accept(")") || !m_lexer.accept(";")) {
            errorAt("expected );");
        }
        return stmt;
    }
    Type type;
    if (looksLikeDeclaration()) {
        parseType(type);
        stmt->kind = kStmtVar;
        stmt->varType = type;
        stmt->name = m_lexer.next().text;
        if (m_lexer.accept("[")) {
            int size = 0;
            if (!m_lexer.accept("]")) {
                Token n = m_lexer.next();
                size = static_cast<int>(n.number);
                if (!m_lexer.accept("]")) {
                    errorAt("expected ]");
                }
            }
            stmt->varType = Type::arrayType(type, size);
        }
        if (m_lexer.accept("=")) {
            stmt->expr = parseExpr();
        }
        if (m_lexer.accept(",")) {
            std::unique_ptr<Stmt> block(new Stmt());
            block->kind = kStmtBlock;
            block->kids.push_back(std::move(stmt));
            do {
                std::unique_ptr<Stmt> extra(new Stmt());
                extra->kind = kStmtVar;
                extra->varType = type;
                extra->name = m_lexer.next().text;
                if (m_lexer.accept("=")) {
                    extra->expr = parseExpr();
                }
                block->kids.push_back(std::move(extra));
            } while (m_lexer.accept(","));
            stmt = std::move(block);
        }
        if (!m_lexer.accept(";")) {
            errorAt("expected ;");
        }
        return stmt;
    }
    stmt->kind = kStmtExpr;
    stmt->expr = parseExpr();
    if (!m_lexer.accept(";")) {
        errorAt("expected ; after expression");
    }
    return stmt;
}

bool
Parser::parseStruct()
{
    m_lexer.next();
    Token name = m_lexer.next();
    Type st;
    st.kind = kTypeStruct;
    st.name = name.text;
    if (!m_lexer.accept("{")) {
        errorAt("expected {");
        return false;
    }
    while (!m_lexer.accept("}") && m_lexer.peek().kind != kTokEof) {
        Type fieldType;
        if (!parseType(fieldType)) {
            errorAt("expected field type");
            return false;
        }
        Token field = m_lexer.next();
        std::string semantic;
        parseSemantic(semantic);
        if (!semantic.empty()) {
            fieldType.name = semantic;
        }
        st.members.push_back(std::make_pair(field.text, fieldType));
        if (!m_lexer.accept(";")) {
            errorAt("expected ;");
            return false;
        }
    }
    m_lexer.accept(";");
    m_userTypes[st.name] = st;
    if (m_unit) {
        Variable marker;
        marker.name = st.name;
        marker.type = st;
        m_unit->variables.push_back(std::move(marker));
    }
    return true;
}

bool
Parser::parseCBuffer(TranslationUnit &unit)
{
    m_lexer.next();
    if (m_lexer.peek().kind != kTokIdent && m_lexer.peek().kind != kTokKeyword) {
        errorAt("expected constant buffer name");
        return false;
    }
    m_lexer.next();
    std::string registerName;
    parseSemantic(registerName);
    parseRegister(registerName);
    if (!m_lexer.accept("{")) {
        errorAt("expected { in constant buffer");
        return false;
    }
    while (!m_lexer.accept("}") && m_lexer.peek().kind != kTokEof) {
        Type type;
        if (!parseType(type)) {
            errorAt("expected constant buffer field type");
            return false;
        }
        if (m_lexer.peek().kind != kTokIdent && m_lexer.peek().kind != kTokKeyword) {
            errorAt("expected constant buffer field name");
            return false;
        }
        Variable variable;
        variable.name = m_lexer.next().text;
        variable.type = type;
        if (m_lexer.accept("[")) {
            Token size = m_lexer.next();
            variable.type = Type::arrayType(type, static_cast<int>(size.number));
            if (!m_lexer.accept("]")) {
                errorAt("expected ] in constant buffer field");
                return false;
            }
        }
        parseSemantic(variable.semantic);
        if (!m_lexer.accept(";")) {
            errorAt("expected ; after constant buffer field");
            return false;
        }
        unit.variables.push_back(std::move(variable));
    }
    if (!m_lexer.accept(";")) {
        errorAt("expected ; after constant buffer");
        return false;
    }
    return true;
}

bool
Parser::parseFunctionOrVar(const Type &type, const std::string &name)
{
    if (m_lexer.accept("(")) {
        Function fn;
        fn.name = name;
        fn.returnType = type;
        if (!m_lexer.accept(")")) {
            do {
                Parameter param;
                param.isOut = false;
                skipAttributes();
                if (m_lexer.peek().text == "out" || m_lexer.peek().text == "inout") {
                    param.isOut = true;
                }
                if (!parseType(param.type)) {
                    errorAt("expected parameter type");
                    return false;
                }
                if (m_lexer.peek().kind == kTokIdent) {
                    param.name = m_lexer.next().text;
                }
                parseSemantic(param.semantic);
                parseRegister(param.registerName);
                if (m_lexer.accept("=")) {
                    parseExpr();
                }
                fn.params.push_back(param);
            } while (m_lexer.accept(","));
            if (!m_lexer.accept(")")) {
                errorAt("expected )");
                return false;
            }
        }
        parseSemantic(fn.returnSemantic);
        std::vector<Annotation> unused;
        parseAnnotations(unused);
        if (m_lexer.peek().text == "{") {
            fn.body = parseBlock();
        }
        else if (!m_lexer.accept(";")) {
            errorAt("expected function body");
            return false;
        }
        m_unit->functions.push_back(std::move(fn));
        return m_error.empty();
    }
    Variable var;
    var.name = name;
    var.type = type;
    if (m_lexer.accept("[")) {
        int size = 0;
        if (!m_lexer.accept("]")) {
            Token n = m_lexer.next();
            size = static_cast<int>(n.number);
            if (!m_lexer.accept("]")) {
                errorAt("expected ]");
                return false;
            }
        }
        var.type = Type::arrayType(type, size);
    }
    parseSemantic(var.semantic);
    parseRegister(var.registerName);
    parseAnnotations(var.annotations);
    if (m_lexer.accept("=")) {
        if (m_lexer.peek().text == "sampler_state" || m_lexer.peek().text == "SamplerState") {
            m_lexer.next();
            if (!m_lexer.accept("{")) {
                errorAt("expected {");
                return false;
            }
            while (!m_lexer.accept("}") && m_lexer.peek().kind != kTokEof) {
                SamplerStateItem item;
                item.key = m_lexer.next().text;
                if (!m_lexer.accept("=")) {
                    errorAt("expected =");
                    return false;
                }
                if (m_lexer.accept("<")) {
                    item.value = m_lexer.next().text;
                    m_lexer.accept(">");
                    var.textureName = item.value;
                }
                else {
                    Token value = m_lexer.next();
                    item.value = value.text;
                    if (m_lexer.accept("(")) {
                        int depth = 1;
                        while (depth > 0 && m_lexer.peek().kind != kTokEof) {
                            if (m_lexer.accept("(")) {
                                depth++;
                            }
                            else if (m_lexer.accept(")")) {
                                depth--;
                            }
                            else {
                                m_lexer.next();
                            }
                        }
                    }
                }
                var.samplerStates.push_back(item);
                m_lexer.accept(";");
            }
        }
        else {
            var.initializer = parseExpr();
        }
    }
    parseAnnotations(var.annotations);
    if (!m_lexer.accept(";")) {
        errorAt("expected ; after variable " + name);
        return false;
    }
    m_unit->variables.push_back(std::move(var));
    return true;
}

bool
Parser::parsePass(Technique &technique)
{
    if (!m_lexer.accept("pass")) {
        errorAt("expected pass");
        return false;
    }
    Pass pass;
    if (m_lexer.peek().kind == kTokIdent || m_lexer.peek().kind == kTokKeyword) {
        pass.name = m_lexer.next().text;
    }
    else {
        pass.name = "p0";
    }
    parseAnnotations(pass.annotations);
    if (!m_lexer.accept("{")) {
        errorAt("expected {");
        return false;
    }
    while (!m_lexer.accept("}") && m_lexer.peek().kind != kTokEof) {
        Token name = m_lexer.next();
        if (name.text == "pass") {
            errorAt("nested pass");
            return false;
        }
        std::string stateName = name.text;
        if (m_lexer.accept("[")) {
            Token index = m_lexer.next();
            stateName += index.text;
            if (!m_lexer.accept("]")) {
                errorAt("expected ]");
                return false;
            }
        }
        if (!m_lexer.accept("=")) {
            errorAt("expected = in pass");
            return false;
        }
        PassState state;
        state.name = stateName;
        if (m_lexer.accept("compile")) {
            Token profile = m_lexer.next();
            state.compileProfile = profile.text;
            Token entry = m_lexer.next();
            state.compileEntry = entry.text;
            if (m_lexer.accept("(")) {
                int depth = 1;
                while (depth > 0 && m_lexer.peek().kind != kTokEof) {
                    if (m_lexer.accept("(")) {
                        depth++;
                    }
                    else if (m_lexer.accept(")")) {
                        depth--;
                    }
                    else {
                        m_lexer.next();
                    }
                }
            }
            if (state.name == "VertexShader" || state.name == "vertexshader") {
                pass.vsEntry = state.compileEntry;
            }
            else if (state.name == "PixelShader" || state.name == "pixelshader") {
                pass.psEntry = state.compileEntry;
            }
        }
        else {
            Token value = m_lexer.next();
            state.value = value.text;
            if (m_lexer.accept("(")) {
                int depth = 1;
                while (depth > 0 && m_lexer.peek().kind != kTokEof) {
                    if (m_lexer.accept("(")) {
                        depth++;
                    }
                    else if (m_lexer.accept(")")) {
                        depth--;
                    }
                    else {
                        m_lexer.next();
                    }
                }
            }
        }
        pass.states.push_back(state);
        m_lexer.accept(";");
    }
    technique.passes.push_back(std::move(pass));
    return true;
}

bool
Parser::parseTechnique(TranslationUnit &unit)
{
    m_lexer.next();
    Technique technique;
    if (m_lexer.peek().kind == kTokIdent || m_lexer.peek().kind == kTokKeyword) {
        technique.name = m_lexer.next().text;
    }
    else {
        technique.name = "t0";
    }
    parseAnnotations(technique.annotations);
    if (!m_lexer.accept("{")) {
        errorAt("expected {");
        return false;
    }
    while (!m_lexer.accept("}") && m_lexer.peek().kind != kTokEof) {
        if (m_lexer.peek().text == "pass") {
            if (!parsePass(technique)) {
                return false;
            }
        }
        else {
            m_lexer.next();
        }
    }
    unit.techniques.push_back(std::move(technique));
    return true;
}

bool
Parser::parseDecl(TranslationUnit &unit)
{
    skipAttributes();
    Token token = m_lexer.peek();
    if (token.kind == kTokEof) {
        return false;
    }
    if (token.text == ";") {
        m_lexer.next();
        return true;
    }
    if (token.text == "technique") {
        return parseTechnique(unit);
    }
    if (token.text == "struct") {
        return parseStruct();
    }
    if (token.text == "cbuffer") {
        return parseCBuffer(unit);
    }
    if (token.text == "typedef") {
        m_lexer.next();
        Type t;
        parseType(t);
        m_lexer.next();
        m_lexer.accept(";");
        return true;
    }
    Type type;
    if (!parseType(type)) {
        errorAt("expected declaration");
        return false;
    }
    if (m_lexer.peek().kind != kTokIdent && m_lexer.peek().kind != kTokKeyword) {
        errorAt("expected identifier");
        return false;
    }
    std::string name = m_lexer.next().text;
    return parseFunctionOrVar(type, name);
}

bool
Parser::parse(const std::string &source, const std::string &filename, TranslationUnit &unit, std::string &error)
{
    m_unit = &unit;
    m_error.clear();
    m_userTypes.clear();
    m_lexer.setSource(source, filename);
    while (m_lexer.peek().kind != kTokEof) {
        if (!parseDecl(unit)) {
            break;
        }
        if (!m_error.empty()) {
            break;
        }
    }
    if (m_lexer.hasError()) {
        m_error = m_lexer.error();
    }
    error = m_error;
    return m_error.empty();
}

} /* namespace fx9next */

/*
   Copyright (c) 2015-2023 hkrn All rights reserved

   This file is licensed under MIT license. for more details, see LICENSE.txt.
 */

#pragma once
#ifndef FX9NEXT_LEXER_H_
#define FX9NEXT_LEXER_H_

#include <string>
#include <vector>

namespace fx9next {

enum TokenKind {
    kTokEof,
    kTokIdent,
    kTokNumber,
    kTokString,
    kTokKeyword,
    kTokPunct
};

struct Token {
    TokenKind kind;
    std::string text;
    double number;
    bool isFloat;
    int line;
    int column;
};

class Lexer {
public:
    void setSource(const std::string &source, const std::string &filename);
    Token peek() const;
    Token peek(size_t ahead) const;
    Token next();
    bool accept(const char *text);
    bool acceptKind(TokenKind kind);
    void expect(const char *text, std::string &error);
    const std::string &filename() const;
    bool hasError() const;
    const std::string &error() const;

private:
    void skipTrivia();
    Token scan();
    Token make(TokenKind kind, const std::string &text, int line, int column) const;
    bool isKeyword(const std::string &ident) const;

    std::string m_source;
    std::string m_filename;
    size_t m_pos;
    int m_line;
    int m_column;
    std::vector<Token> m_lookahead;
    std::string m_error;
};

} /* namespace fx9next */

#endif /* FX9NEXT_LEXER_H_ */

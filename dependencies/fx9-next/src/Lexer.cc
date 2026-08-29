/*
   Copyright (c) 2015-2023 hkrn All rights reserved

   This file is licensed under MIT license. for more details, see LICENSE.txt.
 */

#include "fx9next/Lexer.h"
#include "fx9next/Type.h"

#include <cctype>
#include <cstdlib>
#include <cstring>

namespace fx9next {
namespace {

bool
isIdentStart(unsigned char c)
{
    return std::isalpha(c) || c == '_';
}

bool
isIdentCont(unsigned char c)
{
    return std::isalnum(c) || c == '_';
}

const char *kKeywords[] = { "technique", "pass", "struct", "return", "if", "else", "for", "while", "do", "discard",
    "break", "continue", "true", "false", "compile", "sampler_state", "SamplerState", "register", "typedef", "cbuffer",
    "const", "static", "uniform", "shared", "row_major", "column_major", "in", "out", "inout", "inline", "precise",
    "volatile", "void", "string",
    "texture", "texture1D", "texture2D", "texture3D", "textureCUBE", "Texture1D", "Texture2D", "Texture3D", "TextureCube", "sampler",
    "SamplerState",
    "sampler1D", "sampler2D", "sampler3D", "samplerCUBE", "samplerCube", "samplerVOLUME", nullptr };

} /* namespace anonymous */

void
Lexer::setSource(const std::string &source, const std::string &filename)
{
    m_source = source;
    m_filename = filename;
    m_pos = 0;
    m_line = 1;
    m_column = 1;
    m_lookahead.clear();
    m_error.clear();
}

Token
Lexer::peek() const
{
    return peek(0);
}

Token
Lexer::peek(size_t ahead) const
{
    Lexer *self = const_cast<Lexer *>(this);
    while (self->m_lookahead.size() <= ahead) {
        self->m_lookahead.push_back(self->scan());
    }
    return self->m_lookahead[ahead];
}

Token
Lexer::next()
{
    if (!m_lookahead.empty()) {
        Token token = m_lookahead.front();
        m_lookahead.erase(m_lookahead.begin());
        return token;
    }
    return scan();
}

bool
Lexer::accept(const char *text)
{
    if (peek().text == text) {
        next();
        return true;
    }
    return false;
}

bool
Lexer::acceptKind(TokenKind kind)
{
    if (peek().kind == kind) {
        next();
        return true;
    }
    return false;
}

void
Lexer::expect(const char *text, std::string &error)
{
    if (!accept(text)) {
        error = m_filename + ":" + std::to_string(peek().line) + ": expected '" + text + "', got '" + peek().text + "'";
    }
}

const std::string &
Lexer::filename() const
{
    return m_filename;
}

bool
Lexer::hasError() const
{
    return !m_error.empty();
}

const std::string &
Lexer::error() const
{
    return m_error;
}

void
Lexer::skipTrivia()
{
    while (m_pos < m_source.size()) {
        char c = m_source[m_pos];
        if (c == ' ' || c == '\t' || c == '\r') {
            m_pos++;
            m_column++;
            continue;
        }
        if (c == '\n') {
            m_pos++;
            m_line++;
            m_column = 1;
            continue;
        }
        if (c == '/' && m_pos + 1 < m_source.size() && m_source[m_pos + 1] == '/') {
            m_pos += 2;
            while (m_pos < m_source.size() && m_source[m_pos] != '\n') {
                m_pos++;
            }
            continue;
        }
        if (c == '/' && m_pos + 1 < m_source.size() && m_source[m_pos + 1] == '*') {
            m_pos += 2;
            while (m_pos + 1 < m_source.size() && !(m_source[m_pos] == '*' && m_source[m_pos + 1] == '/')) {
                if (m_source[m_pos] == '\n') {
                    m_line++;
                    m_column = 1;
                }
                m_pos++;
            }
            if (m_pos + 1 < m_source.size()) {
                m_pos += 2;
            }
            continue;
        }
        break;
    }
}

Token
Lexer::make(TokenKind kind, const std::string &text, int line, int column) const
{
    Token token;
    token.kind = kind;
    token.text = text;
    token.number = 0;
    token.isFloat = false;
    token.line = line;
    token.column = column;
    return token;
}

bool
Lexer::isKeyword(const std::string &ident) const
{
    for (const char **p = kKeywords; *p; ++p) {
        if (ident == *p) {
            return true;
        }
    }
    Type dummy;
    if (Type::parseBuiltin(ident, dummy)) {
        return true;
    }
    return false;
}

Token
Lexer::scan()
{
    skipTrivia();
    if (m_pos >= m_source.size()) {
        return make(kTokEof, "", m_line, m_column);
    }
    const int line = m_line;
    const int column = m_column;
    unsigned char c = static_cast<unsigned char>(m_source[m_pos]);
    if (isIdentStart(c)) {
        size_t start = m_pos;
        m_pos++;
        m_column++;
        while (m_pos < m_source.size() && isIdentCont(static_cast<unsigned char>(m_source[m_pos]))) {
            m_pos++;
            m_column++;
        }
        std::string ident = m_source.substr(start, m_pos - start);
        Token token = make(isKeyword(ident) ? kTokKeyword : kTokIdent, ident, line, column);
        return token;
    }
    if (std::isdigit(c) || (c == '.' && m_pos + 1 < m_source.size() && std::isdigit(static_cast<unsigned char>(m_source[m_pos + 1])))) {
        size_t start = m_pos;
        bool isFloat = false;
        if (c == '0' && m_pos + 1 < m_source.size() && (m_source[m_pos + 1] == 'x' || m_source[m_pos + 1] == 'X')) {
            m_pos += 2;
            m_column += 2;
            while (m_pos < m_source.size() && std::isxdigit(static_cast<unsigned char>(m_source[m_pos]))) {
                m_pos++;
                m_column++;
            }
            Token token = make(kTokNumber, m_source.substr(start, m_pos - start), line, column);
            token.number = static_cast<double>(std::strtoul(token.text.c_str(), nullptr, 16));
            return token;
        }
        while (m_pos < m_source.size() && std::isdigit(static_cast<unsigned char>(m_source[m_pos]))) {
            m_pos++;
            m_column++;
        }
        if (m_pos < m_source.size() && m_source[m_pos] == '.') {
            isFloat = true;
            m_pos++;
            m_column++;
            while (m_pos < m_source.size() && std::isdigit(static_cast<unsigned char>(m_source[m_pos]))) {
                m_pos++;
                m_column++;
            }
        }
        if (m_pos < m_source.size() && (m_source[m_pos] == 'e' || m_source[m_pos] == 'E')) {
            isFloat = true;
            m_pos++;
            m_column++;
            if (m_pos < m_source.size() && (m_source[m_pos] == '+' || m_source[m_pos] == '-')) {
                m_pos++;
                m_column++;
            }
            while (m_pos < m_source.size() && std::isdigit(static_cast<unsigned char>(m_source[m_pos]))) {
                m_pos++;
                m_column++;
            }
        }
        if (m_pos < m_source.size() && (m_source[m_pos] == 'f' || m_source[m_pos] == 'F' || m_source[m_pos] == 'h' ||
                                          m_source[m_pos] == 'H')) {
            isFloat = true;
            m_pos++;
            m_column++;
        }
        Token token = make(kTokNumber, m_source.substr(start, m_pos - start), line, column);
        token.isFloat = isFloat;
        token.number = std::strtod(token.text.c_str(), nullptr);
        return token;
    }
    if (c == '"' || c == '\'') {
        char quote = static_cast<char>(c);
        m_pos++;
        m_column++;
        std::string value;
        while (m_pos < m_source.size() && m_source[m_pos] != quote) {
            if (m_source[m_pos] == '\\' && m_pos + 1 < m_source.size()) {
                m_pos++;
                m_column++;
                char esc = m_source[m_pos];
                if (esc == 'n') {
                    value.push_back('\n');
                }
                else if (esc == 't') {
                    value.push_back('\t');
                }
                else {
                    value.push_back(esc);
                }
                m_pos++;
                m_column++;
                continue;
            }
            if (m_source[m_pos] == '\n') {
                m_line++;
                m_column = 1;
            }
            value.push_back(m_source[m_pos]);
            m_pos++;
            m_column++;
        }
        if (m_pos < m_source.size()) {
            m_pos++;
            m_column++;
        }
        return make(kTokString, value, line, column);
    }
    static const char *kMulti[] = { "==", "!=", "<=", ">=", "&&", "||", "<<", ">>", "++", "--", "+=", "-=", "*=", "/=",
        "%=", "##", nullptr };
    for (const char **p = kMulti; *p; ++p) {
        size_t n = std::strlen(*p);
        if (m_pos + n <= m_source.size() && m_source.compare(m_pos, n, *p) == 0) {
            m_pos += n;
            m_column += static_cast<int>(n);
            return make(kTokPunct, *p, line, column);
        }
    }
    std::string one(1, m_source[m_pos]);
    m_pos++;
    m_column++;
    return make(kTokPunct, one, line, column);
}

} /* namespace fx9next */

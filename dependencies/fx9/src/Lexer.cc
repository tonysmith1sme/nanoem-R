/*
   Copyright (c) 2015-2023 hkrn All rights reserved

   This file is licensed under MIT license. for more details, see LICENSE.txt.
 */

#define ENABLE_HLSL
#include "fx9/Lexer.h"
#include "fx9/Parser.h"

#include "Syntax.h"

using namespace glslang;

namespace fx9 {

LexerContext::LexerContext(
    TParseContextBase &parserContext, const std::string &rootFileName, TShader::Includer &includer)
    : TPpContext(parserContext, rootFileName, includer)
    , m_previousToken('\n')
{
    m_keywordIdentifiers.insert(std::make_pair("bool", TOKEN_BOOL));
    m_keywordIdentifiers.insert(std::make_pair("false", TOKEN_FALSE));
    m_keywordIdentifiers.insert(std::make_pair("true", TOKEN_TRUE));
    m_keywordIdentifiers.insert(std::make_pair("register", TOKEN_REGISTER));
    m_keywordIdentifiers.insert(std::make_pair("static", TOKEN_STATIC));
    m_keywordIdentifiers.insert(std::make_pair("void", TOKEN_VOID));
    m_keywordIdentifiers.insert(std::make_pair("half", TOKEN_HALF));
    m_keywordIdentifiers.insert(std::make_pair("int", TOKEN_INT));
    m_keywordIdentifiers.insert(std::make_pair("float", TOKEN_FLOAT));
    m_keywordIdentifiers.insert(std::make_pair("shared", TOKEN_SHARED));
    m_keywordIdentifiers.insert(std::make_pair("struct", TOKEN_STRUCT));
    m_keywordIdentifiers.insert(std::make_pair("return", TOKEN_RETURN));
    m_keywordIdentifiers.insert(std::make_pair("if", TOKEN_IF));
    m_keywordIdentifiers.insert(std::make_pair("else", TOKEN_ELSE));
    m_keywordIdentifiers.insert(std::make_pair("while", TOKEN_WHILE));
    m_keywordIdentifiers.insert(std::make_pair("do", TOKEN_DO));
    m_keywordIdentifiers.insert(std::make_pair("for", TOKEN_FOR));
    m_keywordIdentifiers.insert(std::make_pair("bool2", TOKEN_BOOL2));
    m_keywordIdentifiers.insert(std::make_pair("bool2x2", TOKEN_BOOL2x2));
    m_keywordIdentifiers.insert(std::make_pair("bool2x3", TOKEN_BOOL2x3));
    m_keywordIdentifiers.insert(std::make_pair("bool2x4", TOKEN_BOOL2x4));
    m_keywordIdentifiers.insert(std::make_pair("bool3", TOKEN_BOOL3));
    m_keywordIdentifiers.insert(std::make_pair("bool3x2", TOKEN_BOOL3x2));
    m_keywordIdentifiers.insert(std::make_pair("bool3x3", TOKEN_BOOL3x3));
    m_keywordIdentifiers.insert(std::make_pair("bool3x4", TOKEN_BOOL3x4));
    m_keywordIdentifiers.insert(std::make_pair("bool4", TOKEN_BOOL4));
    m_keywordIdentifiers.insert(std::make_pair("bool4x2", TOKEN_BOOL4x2));
    m_keywordIdentifiers.insert(std::make_pair("bool4x3", TOKEN_BOOL4x3));
    m_keywordIdentifiers.insert(std::make_pair("bool4x4", TOKEN_BOOL4x4));
    m_keywordIdentifiers.insert(std::make_pair("half2", TOKEN_HALF2));
    m_keywordIdentifiers.insert(std::make_pair("half2x2", TOKEN_HALF2x2));
    m_keywordIdentifiers.insert(std::make_pair("half2x3", TOKEN_HALF2x3));
    m_keywordIdentifiers.insert(std::make_pair("half2x4", TOKEN_HALF2x4));
    m_keywordIdentifiers.insert(std::make_pair("half3", TOKEN_HALF3));
    m_keywordIdentifiers.insert(std::make_pair("half3x2", TOKEN_HALF3x2));
    m_keywordIdentifiers.insert(std::make_pair("half3x3", TOKEN_HALF3x3));
    m_keywordIdentifiers.insert(std::make_pair("half3x4", TOKEN_HALF3x4));
    m_keywordIdentifiers.insert(std::make_pair("half4", TOKEN_HALF4));
    m_keywordIdentifiers.insert(std::make_pair("half4x2", TOKEN_HALF4x2));
    m_keywordIdentifiers.insert(std::make_pair("half4x3", TOKEN_HALF4x3));
    m_keywordIdentifiers.insert(std::make_pair("half4x4", TOKEN_HALF4x4));
    m_keywordIdentifiers.insert(std::make_pair("int2", TOKEN_INT2));
    m_keywordIdentifiers.insert(std::make_pair("int2x2", TOKEN_INT2x2));
    m_keywordIdentifiers.insert(std::make_pair("int2x3", TOKEN_INT2x3));
    m_keywordIdentifiers.insert(std::make_pair("int2x4", TOKEN_INT2x4));
    m_keywordIdentifiers.insert(std::make_pair("int3", TOKEN_INT3));
    m_keywordIdentifiers.insert(std::make_pair("int3x2", TOKEN_INT3x2));
    m_keywordIdentifiers.insert(std::make_pair("int3x3", TOKEN_INT3x3));
    m_keywordIdentifiers.insert(std::make_pair("int3x4", TOKEN_INT3x4));
    m_keywordIdentifiers.insert(std::make_pair("int4", TOKEN_INT4));
    m_keywordIdentifiers.insert(std::make_pair("int4x2", TOKEN_INT4x2));
    m_keywordIdentifiers.insert(std::make_pair("int4x3", TOKEN_INT4x3));
    m_keywordIdentifiers.insert(std::make_pair("int4x4", TOKEN_INT4x4));
    m_keywordIdentifiers.insert(std::make_pair("float2", TOKEN_FLOAT2));
    m_keywordIdentifiers.insert(std::make_pair("float2x2", TOKEN_FLOAT2x2));
    m_keywordIdentifiers.insert(std::make_pair("float2x3", TOKEN_FLOAT2x3));
    m_keywordIdentifiers.insert(std::make_pair("float2x4", TOKEN_FLOAT2x4));
    m_keywordIdentifiers.insert(std::make_pair("float3", TOKEN_FLOAT3));
    m_keywordIdentifiers.insert(std::make_pair("float3x2", TOKEN_FLOAT3x2));
    m_keywordIdentifiers.insert(std::make_pair("float3x3", TOKEN_FLOAT3x3));
    m_keywordIdentifiers.insert(std::make_pair("float3x4", TOKEN_FLOAT3x4));
    m_keywordIdentifiers.insert(std::make_pair("float4", TOKEN_FLOAT4));
    m_keywordIdentifiers.insert(std::make_pair("float4x2", TOKEN_FLOAT4x2));
    m_keywordIdentifiers.insert(std::make_pair("float4x3", TOKEN_FLOAT4x3));
    m_keywordIdentifiers.insert(std::make_pair("float4x4", TOKEN_FLOAT4x4));
    m_keywordIdentifiers.insert(std::make_pair("Matrix", TOKEN_FLOAT4x4));
    m_keywordIdentifiers.insert(std::make_pair("matrix", TOKEN_FLOAT4x4));
    m_keywordIdentifiers.insert(std::make_pair("sampler", TOKEN_SAMPLER));
    m_keywordIdentifiers.insert(std::make_pair("sampler2D", TOKEN_SAMPLER2D));
    m_keywordIdentifiers.insert(std::make_pair("sampler3D", TOKEN_SAMPLER3D));
    m_keywordIdentifiers.insert(std::make_pair("samplerCUBE", TOKEN_SAMPLERCUBE));
    m_keywordIdentifiers.insert(std::make_pair("sampler_state", TOKEN_SAMPLER_STATE));
    m_keywordIdentifiers.insert(std::make_pair("Texture2D", TOKEN_TEXTURE2D));
    m_keywordIdentifiers.insert(std::make_pair("Texture3D", TOKEN_TEXTURE3D));
    m_keywordIdentifiers.insert(std::make_pair("TextureCUBE", TOKEN_TEXTURECUBE));
    m_keywordIdentifiers.insert(std::make_pair("texture", TOKEN_TEXTURE));
    m_keywordIdentifiers.insert(std::make_pair("texture2D", TOKEN_TEXTURE2D));
    m_keywordIdentifiers.insert(std::make_pair("texture3D", TOKEN_TEXTURE3D));
    m_keywordIdentifiers.insert(std::make_pair("textureCUBE", TOKEN_TEXTURECUBE));
    m_keywordIdentifiers.insert(std::make_pair("string", TOKEN_STRING));
    m_keywordIdentifiers.insert(std::make_pair("technique", TOKEN_TECHNIQUE));
    m_keywordIdentifiers.insert(std::make_pair("pass", TOKEN_PASS));
    m_keywordIdentifiers.insert(std::make_pair("compile", TOKEN_COMPILE));
    m_keywordIdentifiers.insert(std::make_pair("break", TOKEN_BREAK));
    m_keywordIdentifiers.insert(std::make_pair("continue", TOKEN_CONTINUE));
    m_keywordIdentifiers.insert(std::make_pair("discard", TOKEN_DISCARD));
    m_keywordIdentifiers.insert(std::make_pair("const", TOKEN_CONST));
    m_keywordIdentifiers.insert(std::make_pair("uniform", TOKEN_UNIFORM));
    m_keywordIdentifiers.insert(std::make_pair("inline", TOKEN_INLINE));
    m_keywordIdentifiers.insert(std::make_pair("stateblock", TOKEN_STATEBLOCK));
    m_keywordIdentifiers.insert(std::make_pair("stateblock_state", TOKEN_STATEBLOCK_STATE));
    m_keywordIdentifiers.insert(std::make_pair("in", TOKEN_IN));
    m_keywordIdentifiers.insert(std::make_pair("out", TOKEN_OUT));
    m_keywordIdentifiers.insert(std::make_pair("inout", TOKEN_INOUT));
}

LexerContext::~LexerContext()
{
}

int
LexerContext::scan(const ParserContext *context, TPpToken &token)
{
    int tokenID = 0;
    while (1) {
        tokenID = scanToken(&token);
        tokenID = tokenPaste(tokenID, token);
        switch (tokenID) {
        case '#': {
            if (m_previousToken == '\n') {
                tokenID = readCPPline(&token);
                if (tokenID != -1) {
                    continue;
                }
                missingEndifCheck();
            }
            TPpToken stringifiedToken;
            int stringifiedTokenID = scanToken(&stringifiedToken);
            while (stringifiedTokenID == ' ' || stringifiedTokenID == '\n' || stringifiedTokenID == '\r') {
                stringifiedTokenID = scanToken(&stringifiedToken);
            }
            /* legacy MME effects rely on macro stringification such as #pass_ inside function-like macros */
            if (stringifiedTokenID == PpAtomIdentifier || stringifiedTokenID == PpAtomConstInt ||
                stringifiedTokenID == PpAtomConstUint || stringifiedTokenID == PpAtomConstFloat ||
                stringifiedTokenID == PpAtomConstFloat16 || stringifiedTokenID == PpAtomConstDouble ||
                stringifiedTokenID == PpAtomConstString) {
                token = stringifiedToken;
                snprintf(token.name, sizeof(token.name), "\"%s\"", stringifiedToken.name);
                m_previousToken = PpAtomConstString;
                return TOKEN_STRING;
            }
            return 0;
        }
        case '\n':
        case '\r':
            m_previousToken = '\n';
            continue;
        case ';':
            m_previousToken = tokenID;
            return TOKEN_SEMICOLON;
        case ',':
            m_previousToken = tokenID;
            return TOKEN_COMMA;
        case ':':
            m_previousToken = tokenID;
            return TOKEN_COLON;
        case '=':
            m_previousToken = tokenID;
            return TOKEN_ASSIGN;
        case '(':
            m_previousToken = tokenID;
            return TOKEN_LPAREN;
        case ')':
            m_previousToken = tokenID;
            return TOKEN_RPAREN;
        case '.':
            m_previousToken = tokenID;
            return TOKEN_DOT;
        case '!':
            m_previousToken = tokenID;
            return TOKEN_EXCLAMATION;
        case '-':
            m_previousToken = tokenID;
            return TOKEN_MINUS;
        case '~':
            m_previousToken = tokenID;
            return TOKEN_TILDE;
        case '+':
            m_previousToken = tokenID;
            return TOKEN_PLUS;
        case '*':
            m_previousToken = tokenID;
            return TOKEN_STAR;
        case '/':
            m_previousToken = tokenID;
            return TOKEN_SLASH;
        case '%':
            m_previousToken = tokenID;
            return TOKEN_PERCENT;
        case '<':
            m_previousToken = tokenID;
            return TOKEN_LT;
        case '>':
            m_previousToken = tokenID;
            return TOKEN_GT;
        case '|':
            m_previousToken = tokenID;
            return TOKEN_OR;
        case '^':
            m_previousToken = tokenID;
            return TOKEN_XOR;
        case '&':
            m_previousToken = tokenID;
            return TOKEN_AND;
        case '?':
            m_previousToken = tokenID;
            return TOKEN_QUESTION;
        case '[':
            m_previousToken = tokenID;
            return TOKEN_LBRACKET;
        case ']':
            m_previousToken = tokenID;
            return TOKEN_RBRACKET;
        case '{':
            m_previousToken = tokenID;
            return TOKEN_LBRACE;
        case '}':
            m_previousToken = tokenID;
            return TOKEN_RBRACE;
        case '\\':
            return 0;
        case PPAtomAddAssign:
            m_previousToken = tokenID;
            return TOKEN_ADDASSIGN;
        case PPAtomSubAssign:
            m_previousToken = tokenID;
            return TOKEN_SUBASSIGN;
        case PPAtomMulAssign:
            m_previousToken = tokenID;
            return TOKEN_MULASSIGN;
        case PPAtomDivAssign:
            m_previousToken = tokenID;
            return TOKEN_DIVASSIGN;
        case PPAtomModAssign:
            m_previousToken = tokenID;
            return TOKEN_MODASSIGN;
        case PpAtomRight:
            m_previousToken = tokenID;
            return TOKEN_LSHIFT;
        case PpAtomLeft:
            m_previousToken = tokenID;
            return TOKEN_RSHIFT;
        case PpAtomRightAssign:
            m_previousToken = tokenID;
            return TOKEN_LSHIFTASSIGN;
        case PpAtomLeftAssign:
            m_previousToken = tokenID;
            return TOKEN_RSHIFTASSIGN;
        case PpAtomAndAssign:
            m_previousToken = tokenID;
            return TOKEN_ANDASSIGN;
        case PpAtomOrAssign:
            m_previousToken = tokenID;
            return TOKEN_ORASSIGN;
        case PpAtomXorAssign:
            m_previousToken = tokenID;
            return TOKEN_XORASSIGN;
        case PpAtomAnd:
            m_previousToken = tokenID;
            return TOKEN_ANDAND;
        case PpAtomOr:
            m_previousToken = tokenID;
            return TOKEN_OROR;
        case PpAtomXor:
            m_previousToken = tokenID;
            return TOKEN_XOR;
        case PpAtomEQ:
            m_previousToken = tokenID;
            return TOKEN_EQ;
        case PpAtomGE:
            m_previousToken = tokenID;
            return TOKEN_GE;
        case PpAtomNE:
            m_previousToken = tokenID;
            return TOKEN_NE;
        case PpAtomLE:
            m_previousToken = tokenID;
            return TOKEN_LE;
        case PpAtomDecrement:
            m_previousToken = tokenID;
            return TOKEN_MINUSMINUS;
        case PpAtomIncrement:
            m_previousToken = tokenID;
            return TOKEN_PLUSPLUS;
        case PpAtomConstInt:
            m_previousToken = tokenID;
            return TOKEN_INT_LIT;
        case PpAtomConstUint:
            m_previousToken = tokenID;
            return TOKEN_INT_LIT;
        case PpAtomConstFloat:
        case PpAtomConstFloat16:
            m_previousToken = tokenID;
            return TOKEN_FLOAT_LIT;
        case PpAtomConstDouble:
            m_previousToken = tokenID;
            return TOKEN_FLOAT_LIT;
        case PpAtomConstString:
            m_previousToken = tokenID;
            return TOKEN_STRING;
        case PpAtomIdentifier: {
            switch (MacroExpand(&token, false, true)) {
            case MacroExpandNotStarted:
                break;
            case MacroExpandStarted:
            case MacroExpandUndef:
                continue;
            case MacroExpandError:
                return 0;
            }
            auto it = m_keywordIdentifiers.find(token.name);
            if (it != m_keywordIdentifiers.end()) {
                m_previousToken = tokenID;
                return it->second;
            }
            else if (context->isUserDefinedType(token.name)) {
                m_previousToken = tokenID;
                return TOKEN_TYPE_NAME;
            }
            else {
                m_previousToken = tokenID;
                return TOKEN_ID;
            }
        }
        default:
            return 0;
        }
    }
}

} /* namespace fx9 */

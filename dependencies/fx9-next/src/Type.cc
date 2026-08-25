/*
   Copyright (c) 2015-2023 hkrn All rights reserved

   This file is licensed under MIT license. for more details, see LICENSE.txt.
 */

#include "fx9next/Type.h"

#include <cctype>
#include <sstream>

namespace fx9next {

Type::Type()
    : kind(kTypeVoid)
    , scalar(kTypeFloat)
    , rows(1)
    , columns(1)
    , arraySize(0)
    , samplerDim(kSamplerUnknown)
{
}

Type
Type::voidType()
{
    Type t;
    t.kind = kTypeVoid;
    return t;
}

Type
Type::boolType()
{
    Type t;
    t.kind = kTypeBool;
    t.scalar = kTypeBool;
    return t;
}

Type
Type::intType()
{
    Type t;
    t.kind = kTypeInt;
    t.scalar = kTypeInt;
    return t;
}

Type
Type::floatType()
{
    Type t;
    t.kind = kTypeFloat;
    t.scalar = kTypeFloat;
    return t;
}

Type
Type::stringType()
{
    Type t;
    t.kind = kTypeString;
    return t;
}

Type
Type::vectorType(TypeKind scalarKind, int n)
{
    Type t;
    t.kind = kTypeVector;
    t.scalar = scalarKind;
    t.rows = n;
    t.columns = 1;
    return t;
}

Type
Type::matrixType(TypeKind scalarKind, int r, int c)
{
    Type t;
    t.kind = kTypeMatrix;
    t.scalar = scalarKind;
    t.rows = r;
    t.columns = c;
    return t;
}

Type
Type::samplerType(SamplerDim dim)
{
    Type t;
    t.kind = kTypeSampler;
    t.samplerDim = dim;
    return t;
}

Type
Type::textureType(SamplerDim dim)
{
    Type t;
    t.kind = kTypeTexture;
    t.samplerDim = dim;
    return t;
}

Type
Type::arrayType(const Type &element, int size)
{
    Type t = element;
    t.kind = kTypeArray;
    t.arraySize = size;
    return t;
}

bool
Type::parseBuiltin(const std::string &name, Type &out)
{
    if (name == "void") {
        out = voidType();
        return true;
    }
    if (name == "bool") {
        out = boolType();
        return true;
    }
    if (name == "int" || name == "dword") {
        out = intType();
        return true;
    }
    if (name == "uint") {
        out = intType();
        out.kind = kTypeUInt;
        out.scalar = kTypeUInt;
        return true;
    }
    if (name == "float" || name == "half" || name == "double") {
        out = floatType();
        return true;
    }
    if (name == "string") {
        out = stringType();
        return true;
    }
    if (name == "sampler" || name == "sampler2D") {
        out = samplerType(kSampler2D);
        return true;
    }
    if (name == "sampler1D") {
        out = samplerType(kSampler1D);
        return true;
    }
    if (name == "sampler3D" || name == "samplerVOLUME") {
        out = samplerType(kSampler3D);
        return true;
    }
    if (name == "samplerCUBE" || name == "samplerCube") {
        out = samplerType(kSamplerCube);
        return true;
    }
    if (name == "texture" || name == "texture2D" || name == "Texture2D") {
        out = textureType(kSampler2D);
        return true;
    }
    if (name == "texture3D" || name == "Texture3D") {
        out = textureType(kSampler3D);
        return true;
    }
    if (name == "textureCUBE" || name == "TextureCube") {
        out = textureType(kSamplerCube);
        return true;
    }
    if (name.size() >= 5 && (name.compare(0, 5, "float") == 0 || name.compare(0, 4, "half") == 0 ||
                                name.compare(0, 3, "int") == 0 || name.compare(0, 4, "bool") == 0)) {
        TypeKind scalarKind = kTypeFloat;
        size_t prefix = 5;
        if (name.compare(0, 4, "half") == 0) {
            prefix = 4;
        }
        else if (name.compare(0, 3, "int") == 0) {
            scalarKind = kTypeInt;
            prefix = 3;
        }
        else if (name.compare(0, 4, "bool") == 0) {
            scalarKind = kTypeBool;
            prefix = 4;
        }
        else if (name.compare(0, 5, "float") != 0) {
            return false;
        }
        if (name.size() == prefix + 1 && name[prefix] >= '1' && name[prefix] <= '4') {
            out = vectorType(scalarKind, name[prefix] - '0');
            return true;
        }
        if (name.size() == prefix + 3 && name[prefix] >= '1' && name[prefix] <= '4' && name[prefix + 1] == 'x' &&
            name[prefix + 2] >= '1' && name[prefix + 2] <= '4') {
            out = matrixType(scalarKind, name[prefix] - '0', name[prefix + 2] - '0');
            return true;
        }
    }
    return false;
}

bool
Type::isVoid() const
{
    return kind == kTypeVoid;
}

bool
Type::isNumeric() const
{
    return kind == kTypeBool || kind == kTypeInt || kind == kTypeUInt || kind == kTypeFloat || kind == kTypeVector ||
        kind == kTypeMatrix;
}

bool
Type::isVector() const
{
    return kind == kTypeVector;
}

bool
Type::isMatrix() const
{
    return kind == kTypeMatrix;
}

bool
Type::isSampler() const
{
    return kind == kTypeSampler;
}

int
Type::componentCount() const
{
    if (kind == kTypeVector) {
        return rows;
    }
    if (kind == kTypeMatrix) {
        return rows * columns;
    }
    return 1;
}

std::string
Type::toString() const
{
    std::ostringstream os;
    switch (kind) {
    case kTypeVoid:
        os << "void";
        break;
    case kTypeBool:
        os << "bool";
        break;
    case kTypeInt:
        os << "int";
        break;
    case kTypeUInt:
        os << "uint";
        break;
    case kTypeFloat:
        os << "float";
        break;
    case kTypeString:
        os << "string";
        break;
    case kTypeVector:
        os << (scalar == kTypeInt ? "int" : scalar == kTypeBool ? "bool" : "float") << rows;
        break;
    case kTypeMatrix:
        os << "float" << rows << "x" << columns;
        break;
    case kTypeSampler:
        os << "sampler";
        break;
    case kTypeTexture:
        os << "texture";
        break;
    case kTypeStruct:
        os << name;
        break;
    case kTypeArray:
        os << "array";
        break;
    }
    return os.str();
}

} /* namespace fx9next */

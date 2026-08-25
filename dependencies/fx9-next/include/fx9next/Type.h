/*
   Copyright (c) 2015-2023 hkrn All rights reserved

   This file is licensed under MIT license. for more details, see LICENSE.txt.
 */

#pragma once
#ifndef FX9NEXT_TYPE_H_
#define FX9NEXT_TYPE_H_

#include <string>
#include <vector>

namespace fx9next {

enum TypeKind {
    kTypeVoid,
    kTypeBool,
    kTypeInt,
    kTypeUInt,
    kTypeFloat,
    kTypeString,
    kTypeVector,
    kTypeMatrix,
    kTypeStruct,
    kTypeArray,
    kTypeSampler,
    kTypeTexture
};

enum SamplerDim {
    kSamplerUnknown,
    kSampler1D,
    kSampler2D,
    kSampler3D,
    kSamplerCube
};

struct Type {
    TypeKind kind;
    TypeKind scalar;
    int rows;
    int columns;
    int arraySize;
    SamplerDim samplerDim;
    std::string name;
    std::vector<std::pair<std::string, Type> > members;

    Type();
    static Type voidType();
    static Type boolType();
    static Type intType();
    static Type floatType();
    static Type stringType();
    static Type vectorType(TypeKind scalar, int n);
    static Type matrixType(TypeKind scalar, int rows, int columns);
    static Type samplerType(SamplerDim dim);
    static Type textureType(SamplerDim dim);
    static Type arrayType(const Type &element, int size);
    static bool parseBuiltin(const std::string &name, Type &out);

    bool isVoid() const;
    bool isNumeric() const;
    bool isVector() const;
    bool isMatrix() const;
    bool isSampler() const;
    int componentCount() const;
    std::string toString() const;
};

} /* namespace fx9next */

#endif /* FX9NEXT_TYPE_H_ */

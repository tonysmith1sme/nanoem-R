/*
   Copyright (c) 2015-2023 hkrn All rights reserved

   This file is licensed under MIT license. for more details, see LICENSE.txt.
 */

#include "fx9next/SpirvEmitter.h"

#include <cctype>
#include <cstdlib>
#include <cstring>
#include <unordered_map>

namespace fx9next {
namespace {

enum {
    kOpNop = 0,
    kOpUndef = 1,
    kOpSource = 3,
    kOpName = 5,
    kOpMemberName = 6,
    kOpExtInstImport = 11,
    kOpExtInst = 12,
    kOpMemoryModel = 14,
    kOpEntryPoint = 15,
    kOpExecutionMode = 16,
    kOpCapability = 17,
    kOpTypeVoid = 19,
    kOpTypeBool = 20,
    kOpTypeInt = 21,
    kOpTypeFloat = 22,
    kOpTypeVector = 23,
    kOpTypeMatrix = 24,
    kOpTypeImage = 25,
    kOpTypeSampler = 26,
    kOpTypeSampledImage = 27,
    kOpTypeArray = 28,
    kOpTypeRuntimeArray = 29,
    kOpTypeStruct = 30,
    kOpTypePointer = 32,
    kOpTypeFunction = 33,
    kOpConstantTrue = 41,
    kOpConstantFalse = 42,
    kOpConstant = 43,
    kOpConstantComposite = 44,
    kOpFunction = 54,
    kOpFunctionParameter = 55,
    kOpFunctionEnd = 56,
    kOpFunctionCall = 57,
    kOpVariable = 59,
    kOpLoad = 61,
    kOpStore = 62,
    kOpAccessChain = 65,
    kOpDecorate = 71,
    kOpMemberDecorate = 72,
    kOpVectorExtractDynamic = 77,
    kOpVectorShuffle = 79,
    kOpCompositeConstruct = 80,
    kOpCompositeExtract = 81,
    kOpCompositeInsert = 82,
    kOpImageSampleImplicitLod = 87,
    kOpImageSampleExplicitLod = 88,
    kOpConvertFToS = 110,
    kOpConvertSToF = 111,
    kOpFNegate = 127,
    kOpIAdd = 128,
    kOpFAdd = 129,
    kOpISub = 130,
    kOpFSub = 131,
    kOpIMul = 132,
    kOpFMul = 133,
    kOpUDiv = 134,
    kOpSDiv = 135,
    kOpFDiv = 136,
    kOpSMod = 139,
    kOpFMod = 141,
    kOpVectorTimesScalar = 142,
    kOpVectorTimesMatrix = 144,
    kOpMatrixTimesVector = 145,
    kOpMatrixTimesMatrix = 146,
    kOpDot = 148,
    kOpSelect = 169,
    kOpLogicalNot = 168,
    kOpIEqual = 170,
    kOpINotEqual = 171,
    kOpSGreaterThan = 173,
    kOpSGreaterThanEqual = 175,
    kOpSLessThan = 177,
    kOpSLessThanEqual = 179,
    kOpFOrdEqual = 180,
    kOpFOrdNotEqual = 182,
    kOpFOrdLessThan = 184,
    kOpFOrdGreaterThan = 186,
    kOpFOrdLessThanEqual = 188,
    kOpFOrdGreaterThanEqual = 190,
    kOpPhi = 245,
    kOpLoopMerge = 246,
    kOpSelectionMerge = 247,
    kOpLabel = 248,
    kOpBranch = 249,
    kOpBranchConditional = 250,
    kOpReturn = 253,
    kOpReturnValue = 254,
    kOpUnreachable = 255,
    kOpKill = 252
};

enum {
    kGlslFract = 10,
    kGlslFloor = 8,
    kGlslFAbs = 4,
    kGlslSin = 13,
    kGlslCos = 14,
    kGlslPow = 26,
    kGlslExp = 27,
    kGlslSqrt = 31,
    kGlslInverseSqrt = 32,
    kGlslFMin = 37,
    kGlslFMax = 40,
    kGlslFClamp = 43,
    kGlslFMix = 46,
    kGlslStep = 48,
    kGlslLength = 66,
    kGlslCross = 68,
    kGlslNormalize = 69,
    kGlslReflect = 71
};

enum {
    kAddrLogical = 0,
    kMemGLSL450 = 1,
    kStorageFunction = 7,
    kStorageInput = 1,
    kStorageOutput = 3,
    kStorageUniformConstant = 0,
    kStorageUniform = 2,
    kStoragePrivate = 6,
    kDecorationArrayStride = 6,
    kDecorationBlock = 2,
    kDecorationOffset = 35,
    kDecorationLocation = 30,
    kDecorationBinding = 33,
    kDecorationDescriptorSet = 34,
    kDecorationBuiltIn = 11,
    kBuiltInPosition = 0,
    kFunctionControlNone = 0,
    kCapShader = 1,
    kExecVertex = 0,
    kExecFragment = 4,
    kExecOriginUpperLeft = 7,
    kDim2D = 1,
    kDim3D = 2,
    kDimCube = 3
};

enum { kImageOperandsLod = 2 };

struct Builder {
    std::vector<uint32_t> header;
    std::vector<uint32_t> debug;
    std::vector<uint32_t> decorations;
    std::vector<uint32_t> types;
    std::vector<uint32_t> code;
    uint32_t bound;
    uint32_t idVoid;
    uint32_t idBool;
    uint32_t idInt;
    uint32_t idUInt;
    uint32_t idFloat;
    uint32_t idFloat2;
    uint32_t idFloat3;
    uint32_t idFloat4;
    uint32_t idVoidFn;
    uint32_t idTrue;
    uint32_t idFalse;
    uint32_t idGlsl;
    uint32_t idMat3;
    uint32_t idMat4;
    std::unordered_map<std::string, uint32_t> names;
    std::string error;

    Builder()
        : bound(1)
        , idVoid(0)
        , idBool(0)
        , idInt(0)
        , idUInt(0)
        , idFloat(0)
        , idFloat2(0)
        , idFloat3(0)
        , idFloat4(0)
        , idVoidFn(0)
        , idTrue(0)
        , idFalse(0)
        , idGlsl(0)
        , idMat3(0)
        , idMat4(0)
    {
    }

    void
    emitLiteralString(std::vector<uint32_t> &dest, uint16_t op, uint32_t result, const char *text)
    {
        std::vector<uint32_t> ops;
        ops.push_back(result);
        uint32_t packed = 0;
        int shift = 0;
        for (const char *p = text;; p++) {
            packed |= static_cast<uint32_t>(static_cast<unsigned char>(*p)) << shift;
            shift += 8;
            if (shift == 32 || *p == 0) {
                ops.push_back(packed);
                if (*p == 0) {
                    break;
                }
                packed = 0;
                shift = 0;
            }
        }
        emit(dest, op, ops.data(), static_cast<uint16_t>(ops.size()));
    }

    uint32_t
    extInst(uint32_t ty, uint32_t inst, const uint32_t *args, uint16_t n)
    {
        uint32_t id = nextId();
        std::vector<uint32_t> ops;
        ops.push_back(ty);
        ops.push_back(id);
        ops.push_back(idGlsl);
        ops.push_back(inst);
        for (uint16_t i = 0; i < n; i++) {
            ops.push_back(args[i]);
        }
        emit(code, kOpExtInst, ops.data(), static_cast<uint16_t>(ops.size()));
        return id;
    }

    uint32_t
    nextId()
    {
        return bound++;
    }

    void
    emit(std::vector<uint32_t> &dest, uint16_t op, const uint32_t *ops, uint16_t n)
    {
        dest.push_back((static_cast<uint32_t>(n + 1) << 16) | op);
        for (uint16_t i = 0; i < n; i++) {
            dest.push_back(ops[i]);
        }
    }

    void
    emit1(std::vector<uint32_t> &dest, uint16_t op, uint32_t a)
    {
        uint32_t ops[1] = { a };
        emit(dest, op, ops, 1);
    }

    void
    emit2(std::vector<uint32_t> &dest, uint16_t op, uint32_t a, uint32_t b)
    {
        uint32_t ops[2] = { a, b };
        emit(dest, op, ops, 2);
    }

    void
    emit3(std::vector<uint32_t> &dest, uint16_t op, uint32_t a, uint32_t b, uint32_t c)
    {
        uint32_t ops[3] = { a, b, c };
        emit(dest, op, ops, 3);
    }

    void
    emit4(std::vector<uint32_t> &dest, uint16_t op, uint32_t a, uint32_t b, uint32_t c, uint32_t d)
    {
        uint32_t ops[4] = { a, b, c, d };
        emit(dest, op, ops, 4);
    }

    void
    emit5(std::vector<uint32_t> &dest, uint16_t op, uint32_t a, uint32_t b, uint32_t c, uint32_t d, uint32_t e)
    {
        uint32_t ops[5] = { a, b, c, d, e };
        emit(dest, op, ops, 5);
    }

    void
    emit6(std::vector<uint32_t> &dest, uint16_t op, uint32_t a, uint32_t b, uint32_t c, uint32_t d, uint32_t e,
        uint32_t f)
    {
        uint32_t ops[6] = { a, b, c, d, e, f };
        emit(dest, op, ops, 6);
    }

    uint32_t
    typeVoid()
    {
        if (!idVoid) {
            idVoid = nextId();
            emit1(types, kOpTypeVoid, idVoid);
        }
        return idVoid;
    }

    uint32_t
    typeBool()
    {
        if (!idBool) {
            idBool = nextId();
            emit1(types, kOpTypeBool, idBool);
        }
        return idBool;
    }

    uint32_t
    typeInt()
    {
        if (!idInt) {
            idInt = nextId();
            emit3(types, kOpTypeInt, idInt, 32, 1);
        }
        return idInt;
    }

    uint32_t
    typeUInt()
    {
        if (!idUInt) {
            idUInt = nextId();
            emit3(types, kOpTypeInt, idUInt, 32, 0);
        }
        return idUInt;
    }

    uint32_t
    typeFloat()
    {
        if (!idFloat) {
            idFloat = nextId();
            emit2(types, kOpTypeFloat, idFloat, 32);
        }
        return idFloat;
    }

    uint32_t
    typeVec(int n)
    {
        uint32_t *slot = n == 2 ? &idFloat2 : n == 3 ? &idFloat3 : &idFloat4;
        if (!*slot) {
            *slot = nextId();
            emit3(types, kOpTypeVector, *slot, typeFloat(), static_cast<uint32_t>(n));
        }
        return *slot;
    }

    uint32_t
    typeOf(const Type &t)
    {
        if (t.kind == kTypeBool) {
            return typeBool();
        }
        if (t.kind == kTypeInt || t.kind == kTypeUInt) {
            return typeInt();
        }
        if (t.kind == kTypeVector) {
            return typeVec(t.rows);
        }
        if (t.kind == kTypeArray) {
            Type elem = t;
            elem.kind = (t.rows > 1 && t.columns > 1) ? kTypeMatrix : (t.rows > 1 ? kTypeVector : t.scalar);
            if (elem.kind == kTypeVector || elem.kind == kTypeMatrix) {
                elem.arraySize = 0;
            }
            else {
                elem.kind = t.scalar;
                elem.rows = 1;
                elem.columns = 1;
                elem.arraySize = 0;
            }
            uint32_t elemTy = typeOf(elem);
            uint32_t n = static_cast<uint32_t>(t.arraySize > 0 ? t.arraySize : 1);
            uint32_t len = constU32(n);
            uint32_t id = nextId();
            emit3(types, kOpTypeArray, id, elemTy, len);
            uint32_t stride = 4;
            if (elem.kind == kTypeVector) {
                stride = static_cast<uint32_t>(elem.rows * 4);
            }
            else if (elem.kind == kTypeMatrix) {
                stride = static_cast<uint32_t>(elem.rows * elem.columns * 4);
            }
            emit3(decorations, kOpDecorate, id, kDecorationArrayStride, stride);
            return id;
        }
        if (t.kind == kTypeMatrix) {
            uint32_t *slot = (t.rows == 3 && t.columns == 3) ? &idMat3 : &idMat4;
            if (t.rows == 4 && t.columns == 4) {
                slot = &idMat4;
            }
            if (!*slot) {
                uint32_t col = typeVec(t.rows <= 0 ? 4 : t.rows);
                *slot = nextId();
                emit3(types, kOpTypeMatrix, *slot, col, static_cast<uint32_t>(t.columns <= 0 ? 4 : t.columns));
            }
            return *slot;
        }
        return typeFloat();
    }

    uint32_t
    ptrType(uint32_t pointee, uint32_t storage)
    {
        uint32_t id = nextId();
        emit3(types, kOpTypePointer, id, storage, pointee);
        return id;
    }

    uint32_t
    constU32(uint32_t value)
    {
        uint32_t id = nextId();
        emit3(types, kOpConstant, typeUInt(), id, value);
        return id;
    }

    uint32_t
    constI32(int value)
    {
        uint32_t bits = 0;
        std::memcpy(&bits, &value, 4);
        uint32_t id = nextId();
        emit3(types, kOpConstant, typeInt(), id, bits);
        return id;
    }

    uint32_t
    constF32(float value)
    {
        uint32_t bits = 0;
        std::memcpy(&bits, &value, 4);
        uint32_t id = nextId();
        emit3(types, kOpConstant, typeFloat(), id, bits);
        return id;
    }

    uint32_t
    constVec4(float x, float y, float z, float w)
    {
        uint32_t comps[4] = { constF32(x), constF32(y), constF32(z), constF32(w) };
        uint32_t id = nextId();
        uint32_t ops[6] = { typeVec(4), id, comps[0], comps[1], comps[2], comps[3] };
        emit(types, kOpConstantComposite, ops, 6);
        return id;
    }

    uint32_t
    decorate(uint32_t target, uint32_t dec, uint32_t extra, bool hasExtra)
    {
        if (hasExtra) {
            emit3(decorations, kOpDecorate, target, dec, extra);
        }
        else {
            emit2(decorations, kOpDecorate, target, dec);
        }
        return target;
    }

    void
    name(uint32_t target, const char *text)
    {
        emitLiteralString(debug, kOpName, target, text);
    }

    void
    memberName(uint32_t type, uint32_t index, const char *text)
    {
        std::vector<uint32_t> ops;
        ops.push_back(type);
        ops.push_back(index);
        uint32_t packed = 0;
        int shift = 0;
        for (const char *p = text;; p++) {
            packed |= static_cast<uint32_t>(static_cast<unsigned char>(*p)) << shift;
            shift += 8;
            if (shift == 32 || *p == 0) {
                ops.push_back(packed);
                if (*p == 0) {
                    break;
                }
                packed = 0;
                shift = 0;
            }
        }
        emit(debug, kOpMemberName, ops.data(), static_cast<uint16_t>(ops.size()));
    }
};

int
semanticLocation(const std::string &semantic)
{
    std::string u;
    for (size_t i = 0; i < semantic.size(); i++) {
        u.push_back(static_cast<char>(std::toupper(static_cast<unsigned char>(semantic[i]))));
    }
    if (u == "POSITION" || u == "POSITION0" || u == "SV_POSITION") {
        return 0;
    }
    if (u == "NORMAL" || u == "NORMAL0") {
        return 3;
    }
    if (u.compare(0, 8, "TEXCOORD") == 0) {
        int n = 0;
        if (u.size() > 8) {
            n = std::atoi(u.c_str() + 8);
        }
        return 4 + n;
    }
    if (u.compare(0, 5, "COLOR") == 0) {
        int n = 0;
        if (u.size() > 5) {
            n = std::atoi(u.c_str() + 5);
        }
        return 12 + n;
    }
    if (u == "VPOS" || u == "SV_POSITION") {
        return 0;
    }
    return 15;
}

const EffectBindingIR *
findEffectBinding(const EffectModuleIR *effect, const std::string &name, EffectRegisterSetIR set)
{
    if (!effect) {
        return nullptr;
    }
    for (std::vector<EffectBindingIR>::const_iterator it = effect->bindings.begin(); it != effect->bindings.end(); ++it) {
        if (it->name == name && it->registerSet == set) {
            return &*it;
        }
    }
    return nullptr;
}

bool
isPositionSemantic(const std::string &semantic)
{
    std::string u;
    for (size_t i = 0; i < semantic.size(); i++) {
        u.push_back(static_cast<char>(std::toupper(static_cast<unsigned char>(semantic[i]))));
    }
    return u == "POSITION" || u == "POSITION0" || u == "SV_POSITION";
}

int
swizzleIndex(char c)
{
    if (c == 'x' || c == 'r' || c == 's') {
        return 0;
    }
    if (c == 'y' || c == 'g' || c == 't') {
        return 1;
    }
    if (c == 'z' || c == 'b' || c == 'p') {
        return 2;
    }
    if (c == 'w' || c == 'a' || c == 'q') {
        return 3;
    }
    return 0;
}

const Function *
findFunction(const TranslationUnit &unit, const std::string &name)
{
    for (size_t i = 0; i < unit.functions.size(); i++) {
        if (unit.functions[i].name == name) {
            return &unit.functions[i];
        }
    }
    return nullptr;
}

struct Emitter {
    Builder b;
    const TranslationUnit *unit;
    const EffectModuleIR *effect;
    const ShaderModuleIR *shader;
    std::unordered_map<std::string, Type> structTypes;
    SpirvShaderStage stage;
    std::unordered_map<std::string, uint32_t> locals;
    std::unordered_map<std::string, uint32_t> localTypes;
    std::unordered_map<std::string, uint32_t> arrayElems;
    std::unordered_map<std::string, uint32_t> arrayStorage;
    std::unordered_map<std::string, uint32_t> valueOverlay;
    std::unordered_map<std::string, uint32_t> samplers;
    std::unordered_map<std::string, uint32_t> samplerTypes;
    std::unordered_map<std::string, SamplerDim> samplerDims;
    std::unordered_map<std::string, uint32_t> uniformRegisters;
    std::unordered_map<std::string, Type> uniformTypes;
    std::unordered_map<uint32_t, uint32_t> valueTypes;
    uint32_t uniformBuffer = 0;
    uint32_t currentFnType;
    uint32_t pendingReturn;
    std::string pendingStructReturn;
    std::unordered_map<std::string, uint32_t> structOutVars;

    uint32_t
    note(uint32_t id, uint32_t ty)
    {
        valueTypes[id] = ty;
        return id;
    }

    bool
    isVec(uint32_t id) const
    {
        auto it = valueTypes.find(id);
        return it != valueTypes.end() && it->second != b.idFloat && it->second != b.idInt && it->second != b.idBool;
    }

    uint32_t emitExpr(const Expr *expr);
    bool emitStmt(const Stmt *stmt, uint32_t returnType);
    uint32_t loadIdent(const std::string &name);
    uint32_t loadUniform(const std::string &name);
    uint32_t loadUniformElement(const std::string &name, uint32_t index);
    uint32_t narrowUniformValue(const Type &type, uint32_t value);
    uint32_t extractComp(uint32_t vec, uint32_t index);
    uint32_t makeVec3(uint32_t x, uint32_t y, uint32_t z);
    uint32_t makeVec2(uint32_t x, uint32_t y);
    uint32_t sampleTexture(const std::string &sampName, uint32_t uv, uint32_t lod = 0);
    uint32_t callUser(const std::string &name, const Expr *expr);
    uint32_t asBool(uint32_t id);
    uint32_t asInt(uint32_t id);
    uint32_t emitIndexPtr(const Expr *expr, uint32_t &elemTy);
    void emitIf(const Stmt *stmt, uint32_t returnType);
    void emitFor(const Stmt *stmt, uint32_t returnType);
    void emitWhile(const Stmt *stmt, uint32_t returnType, bool doWhile);
    void rememberArray(const std::string &name, const Type &type, uint32_t storage);
    void allocStructLocals(const std::string &name, const Type &st);
    const Type *resolveStruct(const Type &type) const;
    bool isMat(uint32_t id) const;
    uint32_t extractMatrix(uint32_t mat, const std::string &sw);
    uint32_t insertMatrix(uint32_t mat, const std::string &sw, uint32_t value);
    uint32_t splatMatrix(uint32_t scalar, uint32_t matTy, int dim);
};

struct MatComp {
    uint32_t row;
    uint32_t col;
};

bool
parseMatSwizzle(const std::string &s, std::vector<MatComp> &out)
{
    size_t i = 0;
    while (i < s.size()) {
        if (s[i] == '_' && i + 3 < s.size() && (s[i + 1] == 'm' || s[i + 1] == 'M') &&
            std::isdigit(static_cast<unsigned char>(s[i + 2])) &&
            std::isdigit(static_cast<unsigned char>(s[i + 3]))) {
            MatComp c;
            c.row = static_cast<uint32_t>(s[i + 2] - '0');
            c.col = static_cast<uint32_t>(s[i + 3] - '0');
            out.push_back(c);
            i += 4;
            continue;
        }
        if (s[i] == '_' && i + 2 < s.size() && std::isdigit(static_cast<unsigned char>(s[i + 1])) &&
            std::isdigit(static_cast<unsigned char>(s[i + 2]))) {
            MatComp c;
            c.row = static_cast<uint32_t>(s[i + 1] - '1');
            c.col = static_cast<uint32_t>(s[i + 2] - '1');
            out.push_back(c);
            i += 3;
            continue;
        }
        return false;
    }
    return !out.empty();
}

uint32_t
Emitter::extractComp(uint32_t vec, uint32_t index)
{
    if (!isVec(vec)) {
        return vec;
    }
    uint32_t id = b.nextId();
    b.emit4(b.code, kOpCompositeExtract, b.typeFloat(), id, vec, index);
    return note(id, b.typeFloat());
}

uint32_t
Emitter::makeVec3(uint32_t x, uint32_t y, uint32_t z)
{
    uint32_t id = b.nextId();
    uint32_t ops[5] = { b.typeVec(3), id, x, y, z };
    b.emit(b.code, kOpCompositeConstruct, ops, 5);
    return note(id, b.typeVec(3));
}

uint32_t
Emitter::makeVec2(uint32_t x, uint32_t y)
{
    uint32_t id = b.nextId();
    uint32_t ops[4] = { b.typeVec(2), id, x, y };
    b.emit(b.code, kOpCompositeConstruct, ops, 4);
    return note(id, b.typeVec(2));
}

uint32_t
Emitter::sampleTexture(const std::string &sampName, uint32_t uv, uint32_t lod)
{
    auto sit = samplers.find(sampName);
    if (sit == samplers.end()) {
        return note(b.constVec4(0, 0, 0, 1), b.typeVec(4));
    }
    uint32_t loaded = b.nextId();
    uint32_t sampledType = samplerTypes[sampName];
    b.emit3(b.code, kOpLoad, sampledType, loaded, sit->second);
    uint32_t coord = uv;
    const SamplerDim dim = samplerDims[sampName];
    if (dim == kSampler2D && isVec(uv) && valueTypes[uv] == b.typeVec(4)) {
        coord = makeVec2(extractComp(uv, 0), extractComp(uv, 1));
    }
    else if (dim == kSampler2D && isVec(uv) && valueTypes[uv] == b.typeVec(3)) {
        coord = makeVec2(extractComp(uv, 0), extractComp(uv, 1));
    }
    else if ((dim == kSamplerCube || dim == kSampler3D) && isVec(uv) && valueTypes[uv] == b.typeVec(4)) {
        coord = makeVec3(extractComp(uv, 0), extractComp(uv, 1), extractComp(uv, 2));
    }
    uint32_t id = b.nextId();
    if (lod) {
        b.emit6(b.code, kOpImageSampleExplicitLod, b.typeVec(4), id, loaded, coord, kImageOperandsLod, lod);
    }
    else {
        b.emit4(b.code, kOpImageSampleImplicitLod, b.typeVec(4), id, loaded, coord);
    }
    return note(id, b.typeVec(4));
}

uint32_t
Emitter::callUser(const std::string &name, const Expr *expr)
{
    const Function *callee = findFunction(*unit, name);
    if (!callee || !callee->body || name == "main") {
        return 0;
    }
    std::unordered_map<std::string, uint32_t> savedOverlay = valueOverlay;
    size_t argIndex = 1;
    for (size_t i = 0; i < callee->params.size(); i++) {
        uint32_t arg = argIndex < expr->kids.size() ? emitExpr(expr->kids[argIndex].get()) : b.constF32(0);
        argIndex++;
        valueOverlay[callee->params[i].name] = arg;
    }
    uint32_t savedRet = pendingReturn;
    pendingReturn = 0;
    emitStmt(callee->body.get(), 0);
    uint32_t result = pendingReturn ? pendingReturn : b.constF32(0);
    pendingReturn = savedRet;
    valueOverlay = savedOverlay;
    return result;
}

uint32_t
Emitter::asInt(uint32_t id)
{
    if (valueTypes.count(id) && (valueTypes[id] == b.typeInt() || valueTypes[id] == b.typeUInt())) {
        return id;
    }
    uint32_t conv = b.nextId();
    b.emit3(b.code, kOpConvertFToS, b.typeInt(), conv, id);
    return note(conv, b.typeInt());
}

const Type *
Emitter::resolveStruct(const Type &type) const
{
    if (type.kind == kTypeStruct && !type.members.empty()) {
        return &type;
    }
    if (!unit) {
        return nullptr;
    }
    const std::string &key = type.name;
    for (size_t i = 0; i < unit->variables.size(); i++) {
        const Type &st = unit->variables[i].type;
        if (st.kind == kTypeStruct && !st.members.empty() &&
            (st.name == key || unit->variables[i].name == key)) {
            return &st;
        }
    }
    return type.kind == kTypeStruct ? &type : nullptr;
}

void
Emitter::allocStructLocals(const std::string &name, const Type &st)
{
    for (size_t i = 0; i < st.members.size(); i++) {
        const std::string field = name + "." + st.members[i].first;
        uint32_t ty = b.typeOf(st.members[i].second);
        uint32_t ptr = b.ptrType(ty, kStoragePrivate);
        uint32_t var = b.nextId();
        b.emit3(b.types, kOpVariable, ptr, var, kStoragePrivate);
        locals[field] = var;
        localTypes[field] = ty;
        uint32_t zero = (ty == b.typeInt()) ? b.constI32(0) : b.constF32(0);
        if (ty == b.typeVec(2) || ty == b.typeVec(3) || ty == b.typeVec(4)) {
            int n = ty == b.typeVec(2) ? 2 : ty == b.typeVec(3) ? 3 : 4;
            std::vector<uint32_t> comps(static_cast<size_t>(n), b.constF32(0));
            uint32_t id = b.nextId();
            std::vector<uint32_t> ops;
            ops.push_back(ty);
            ops.push_back(id);
            ops.insert(ops.end(), comps.begin(), comps.end());
            b.emit(b.code, kOpCompositeConstruct, ops.data(), static_cast<uint16_t>(ops.size()));
            zero = note(id, ty);
        }
        b.emit2(b.code, kOpStore, var, zero);
    }
}

void
Emitter::rememberArray(const std::string &name, const Type &type, uint32_t storage)
{
    if (type.kind != kTypeArray) {
        return;
    }
    Type elem = type;
    elem.kind = (type.rows > 1 && type.columns > 1) ? kTypeMatrix : (type.rows > 1 ? kTypeVector : type.scalar);
    elem.arraySize = 0;
    if (elem.kind != kTypeVector && elem.kind != kTypeMatrix) {
        elem.kind = type.scalar;
        elem.rows = 1;
        elem.columns = 1;
    }
    arrayElems[name] = b.typeOf(elem);
    arrayStorage[name] = storage;
}

uint32_t
Emitter::emitIndexPtr(const Expr *expr, uint32_t &elemTy)
{
    elemTy = b.typeFloat();
    if (!expr || expr->kids.size() < 2 || expr->kids[0]->kind != kExprIdent) {
        return 0;
    }
    const std::string &name = expr->kids[0]->name;
    auto lit = locals.find(name);
    if (lit == locals.end() || arrayElems.find(name) == arrayElems.end()) {
        return 0;
    }
    elemTy = arrayElems[name];
    uint32_t idx = asInt(emitExpr(expr->kids[1].get()));
    uint32_t storage = arrayStorage.count(name) ? arrayStorage[name] : kStoragePrivate;
    uint32_t ptrTy = b.ptrType(elemTy, storage);
    uint32_t ptr = b.nextId();
    b.emit4(b.code, kOpAccessChain, ptrTy, ptr, lit->second, idx);
    return ptr;
}

bool
Emitter::isMat(uint32_t id) const
{
    auto it = valueTypes.find(id);
    return it != valueTypes.end() && (it->second == b.idMat3 || it->second == b.idMat4);
}

uint32_t
Emitter::extractMatrix(uint32_t mat, const std::string &sw)
{
    std::vector<MatComp> comps;
    if (!parseMatSwizzle(sw, comps)) {
        return mat;
    }
    std::vector<uint32_t> scalars;
    for (size_t i = 0; i < comps.size(); i++) {
        uint32_t id = b.nextId();
        uint32_t ops[5] = { b.typeFloat(), id, mat, comps[i].col, comps[i].row };
        b.emit(b.code, kOpCompositeExtract, ops, 5);
        scalars.push_back(note(id, b.typeFloat()));
    }
    if (scalars.size() == 1) {
        return scalars[0];
    }
    uint32_t id = b.nextId();
    std::vector<uint32_t> ops;
    ops.push_back(b.typeVec(static_cast<int>(scalars.size())));
    ops.push_back(id);
    ops.insert(ops.end(), scalars.begin(), scalars.end());
    b.emit(b.code, kOpCompositeConstruct, ops.data(), static_cast<uint16_t>(ops.size()));
    return note(id, b.typeVec(static_cast<int>(scalars.size())));
}

uint32_t
Emitter::insertMatrix(uint32_t mat, const std::string &sw, uint32_t value)
{
    std::vector<MatComp> comps;
    if (!parseMatSwizzle(sw, comps)) {
        return value;
    }
    uint32_t matTy = valueTypes.count(mat) ? valueTypes[mat] : b.idMat4;
    uint32_t current = mat;
    for (size_t i = 0; i < comps.size(); i++) {
        uint32_t scalar = comps.size() == 1 ? value : extractComp(value, static_cast<uint32_t>(i));
        uint32_t id = b.nextId();
        uint32_t ops[6] = { matTy, id, scalar, current, comps[i].col, comps[i].row };
        b.emit(b.code, kOpCompositeInsert, ops, 6);
        current = note(id, matTy);
    }
    return current;
}

uint32_t
Emitter::splatMatrix(uint32_t scalar, uint32_t matTy, int dim)
{
    std::vector<uint32_t> cols;
    for (int c = 0; c < dim; c++) {
        std::vector<uint32_t> el;
        for (int r = 0; r < dim; r++) {
            el.push_back(scalar);
        }
        uint32_t col = b.nextId();
        std::vector<uint32_t> ops;
        ops.push_back(b.typeVec(dim));
        ops.push_back(col);
        ops.insert(ops.end(), el.begin(), el.end());
        b.emit(b.code, kOpCompositeConstruct, ops.data(), static_cast<uint16_t>(ops.size()));
        note(col, b.typeVec(dim));
        cols.push_back(col);
    }
    uint32_t id = b.nextId();
    std::vector<uint32_t> ops;
    ops.push_back(matTy);
    ops.push_back(id);
    ops.insert(ops.end(), cols.begin(), cols.end());
    b.emit(b.code, kOpCompositeConstruct, ops.data(), static_cast<uint16_t>(ops.size()));
    return note(id, matTy);
}

uint32_t
Emitter::asBool(uint32_t id)
{
    if (valueTypes.count(id) && valueTypes[id] == b.typeBool()) {
        return id;
    }
    uint32_t cond = b.nextId();
    if (valueTypes.count(id) && valueTypes[id] == b.typeInt()) {
        b.emit4(b.code, kOpINotEqual, b.typeBool(), cond, id, b.constI32(0));
    }
    else {
        b.emit4(b.code, kOpFOrdNotEqual, b.typeBool(), cond, id, b.constF32(0));
    }
    return note(cond, b.typeBool());
}

void
Emitter::emitIf(const Stmt *stmt, uint32_t returnType)
{
    uint32_t cond = asBool(emitExpr(stmt->expr.get()));
    uint32_t thenL = b.nextId();
    uint32_t mergeL = b.nextId();
    uint32_t elseL = stmt->elseStmt ? b.nextId() : mergeL;
    b.emit2(b.code, kOpSelectionMerge, mergeL, 0);
    b.emit3(b.code, kOpBranchConditional, cond, thenL, elseL);
    b.emit1(b.code, kOpLabel, thenL);
    if (stmt->thenStmt) {
        emitStmt(stmt->thenStmt.get(), returnType);
    }
    b.emit1(b.code, kOpBranch, mergeL);
    if (stmt->elseStmt) {
        b.emit1(b.code, kOpLabel, elseL);
        emitStmt(stmt->elseStmt.get(), returnType);
        b.emit1(b.code, kOpBranch, mergeL);
    }
    b.emit1(b.code, kOpLabel, mergeL);
}

void
Emitter::emitFor(const Stmt *stmt, uint32_t returnType)
{
    if (!stmt->name.empty()) {
        uint32_t ty = b.typeOf(stmt->varType.kind == kTypeVoid ? Type::floatType() : stmt->varType);
        uint32_t ptr = b.ptrType(ty, kStoragePrivate);
        uint32_t var = b.nextId();
        b.emit3(b.types, kOpVariable, ptr, var, kStoragePrivate);
        locals[stmt->name] = var;
        localTypes[stmt->name] = ty;
        if (stmt->expr) {
            b.emit2(b.code, kOpStore, var, emitExpr(stmt->expr.get()));
        }
    }
    else if (stmt->expr) {
        emitExpr(stmt->expr.get());
    }
    uint32_t header = b.nextId();
    uint32_t condL = b.nextId();
    uint32_t body = b.nextId();
    uint32_t cont = b.nextId();
    uint32_t merge = b.nextId();
    b.emit1(b.code, kOpBranch, header);
    b.emit1(b.code, kOpLabel, header);
    b.emit3(b.code, kOpLoopMerge, merge, cont, 0);
    b.emit1(b.code, kOpBranch, condL);
    b.emit1(b.code, kOpLabel, condL);
    if (stmt->expr2) {
        uint32_t cond = asBool(emitExpr(stmt->expr2.get()));
        b.emit3(b.code, kOpBranchConditional, cond, body, merge);
    }
    else {
        b.emit1(b.code, kOpBranch, body);
    }
    b.emit1(b.code, kOpLabel, body);
    if (stmt->thenStmt) {
        emitStmt(stmt->thenStmt.get(), returnType);
    }
    b.emit1(b.code, kOpBranch, cont);
    b.emit1(b.code, kOpLabel, cont);
    if (stmt->expr3) {
        emitExpr(stmt->expr3.get());
    }
    b.emit1(b.code, kOpBranch, header);
    b.emit1(b.code, kOpLabel, merge);
}

void
Emitter::emitWhile(const Stmt *stmt, uint32_t returnType, bool doWhile)
{
    uint32_t header = b.nextId();
    uint32_t condL = b.nextId();
    uint32_t body = b.nextId();
    uint32_t cont = b.nextId();
    uint32_t merge = b.nextId();
    b.emit1(b.code, kOpBranch, doWhile ? body : header);
    b.emit1(b.code, kOpLabel, header);
    b.emit3(b.code, kOpLoopMerge, merge, cont, 0);
    b.emit1(b.code, kOpBranch, condL);
    b.emit1(b.code, kOpLabel, condL);
    if (stmt->expr) {
        uint32_t cond = asBool(emitExpr(stmt->expr.get()));
        b.emit3(b.code, kOpBranchConditional, cond, body, merge);
    }
    else {
        b.emit1(b.code, kOpBranch, body);
    }
    b.emit1(b.code, kOpLabel, body);
    if (stmt->thenStmt) {
        emitStmt(stmt->thenStmt.get(), returnType);
    }
    b.emit1(b.code, kOpBranch, cont);
    b.emit1(b.code, kOpLabel, cont);
    b.emit1(b.code, kOpBranch, header);
    b.emit1(b.code, kOpLabel, merge);
}

uint32_t
Emitter::loadIdent(const std::string &name)
{
    auto overlay = valueOverlay.find(name);
    if (overlay != valueOverlay.end()) {
        return overlay->second;
    }
    auto it = locals.find(name);
    if (it == locals.end()) {
        return uniformRegisters.count(name) ? loadUniform(name) : b.constF32(0);
    }
    uint32_t resultType = localTypes[name];
    uint32_t id = b.nextId();
    b.emit3(b.code, kOpLoad, resultType, id, it->second);
    return note(id, resultType);
}

uint32_t
Emitter::loadUniform(const std::string &name)
{
    const Type &type = uniformTypes[name];
    const uint32_t vec4 = b.typeVec(4);
    const uint32_t ptr = b.nextId();
    b.emit5(b.code, kOpAccessChain, b.ptrType(vec4, kStorageUniform), ptr, uniformBuffer, b.constI32(0),
        b.constI32(static_cast<int>(uniformRegisters[name])));
    const uint32_t value = b.nextId();
    b.emit3(b.code, kOpLoad, vec4, value, ptr);
    note(value, vec4);
    if (type.kind == kTypeMatrix) {
        std::vector<uint32_t> columns;
        for (int i = 0; i < type.columns; i++) {
            const uint32_t columnPtr = b.nextId();
            b.emit5(b.code, kOpAccessChain, b.ptrType(vec4, kStorageUniform), columnPtr, uniformBuffer, b.constI32(0),
                b.constI32(static_cast<int>(uniformRegisters[name] + i)));
            const uint32_t column = b.nextId();
            b.emit3(b.code, kOpLoad, vec4, column, columnPtr);
            note(column, vec4);
            if (type.rows == 4) {
                columns.push_back(column);
            }
            else {
                std::vector<uint32_t> components;
                for (int j = 0; j < type.rows; j++) {
                    components.push_back(extractComp(column, static_cast<uint32_t>(j)));
                }
                const uint32_t columnType = b.typeVec(type.rows);
                const uint32_t narrowed = b.nextId();
                std::vector<uint32_t> ops;
                ops.push_back(columnType);
                ops.push_back(narrowed);
                ops.insert(ops.end(), components.begin(), components.end());
                b.emit(b.code, kOpCompositeConstruct, ops.data(), static_cast<uint16_t>(ops.size()));
                columns.push_back(note(narrowed, columnType));
            }
        }
        const uint32_t resultType = b.typeOf(type);
        const uint32_t result = b.nextId();
        std::vector<uint32_t> ops;
        ops.push_back(resultType);
        ops.push_back(result);
        ops.insert(ops.end(), columns.begin(), columns.end());
        b.emit(b.code, kOpCompositeConstruct, ops.data(), static_cast<uint16_t>(ops.size()));
        return note(result, resultType);
    }
    if (type.kind == kTypeVector && type.rows < 4) {
        std::vector<uint32_t> components;
        for (int i = 0; i < type.rows; i++) {
            components.push_back(extractComp(value, static_cast<uint32_t>(i)));
        }
        const uint32_t resultType = b.typeOf(type);
        const uint32_t result = b.nextId();
        std::vector<uint32_t> ops;
        ops.push_back(resultType);
        ops.push_back(result);
        ops.insert(ops.end(), components.begin(), components.end());
        b.emit(b.code, kOpCompositeConstruct, ops.data(), static_cast<uint16_t>(ops.size()));
        return note(result, resultType);
    }
    if (type.kind == kTypeFloat || type.kind == kTypeInt || type.kind == kTypeUInt || type.kind == kTypeBool) {
        return extractComp(value, 0);
    }
    return value;
}

uint32_t
Emitter::loadUniformElement(const std::string &name, uint32_t index)
{
    const Type &arrayType = uniformTypes[name];
    Type elementType = arrayType;
    elementType.kind = arrayType.rows > 1 ? (arrayType.rows == 4 ? kTypeVector : kTypeVector) : arrayType.scalar;
    elementType.arraySize = 0;
    const uint32_t slot = b.nextId();
    b.emit4(b.code, kOpIAdd, b.typeInt(), slot, b.constI32(static_cast<int>(uniformRegisters[name])), index);
    note(slot, b.typeInt());
    const uint32_t vec4 = b.typeVec(4);
    const uint32_t ptr = b.nextId();
    b.emit5(b.code, kOpAccessChain, b.ptrType(vec4, kStorageUniform), ptr, uniformBuffer, b.constI32(0), slot);
    const uint32_t value = b.nextId();
    b.emit3(b.code, kOpLoad, vec4, value, ptr);
    note(value, vec4);
    return narrowUniformValue(elementType, value);
}

uint32_t
Emitter::narrowUniformValue(const Type &type, uint32_t value)
{
    if (type.kind == kTypeVector && type.rows < 4) {
        std::vector<uint32_t> components;
        for (int i = 0; i < type.rows; i++) {
            components.push_back(extractComp(value, static_cast<uint32_t>(i)));
        }
        const uint32_t resultType = b.typeOf(type);
        const uint32_t result = b.nextId();
        std::vector<uint32_t> ops;
        ops.push_back(resultType);
        ops.push_back(result);
        ops.insert(ops.end(), components.begin(), components.end());
        b.emit(b.code, kOpCompositeConstruct, ops.data(), static_cast<uint16_t>(ops.size()));
        return note(result, resultType);
    }
    if (type.kind == kTypeFloat) {
        return extractComp(value, 0);
    }
    return value;
}

uint32_t
Emitter::emitExpr(const Expr *expr)
{
    if (!expr) {
        return b.constF32(0);
    }
    switch (expr->kind) {
    case kExprLiteralFloat:
        return note(b.constF32(static_cast<float>(expr->floatValue)), b.typeFloat());
    case kExprLiteralInt:
        return note(b.constI32(expr->intValue), b.typeInt());
    case kExprLiteralBool:
        if (expr->boolValue) {
            if (!b.idTrue) {
                b.idTrue = b.nextId();
                b.emit2(b.types, kOpConstantTrue, b.typeBool(), b.idTrue);
            }
            return b.idTrue;
        }
        if (!b.idFalse) {
            b.idFalse = b.nextId();
            b.emit2(b.types, kOpConstantFalse, b.typeBool(), b.idFalse);
        }
        return b.idFalse;
    case kExprIdent:
        return loadIdent(expr->name);
    case kExprConstruct: {
        std::vector<uint32_t> comps;
        for (size_t i = 0; i < expr->kids.size(); i++) {
            comps.push_back(emitExpr(expr->kids[i].get()));
        }
        int want = expr->type.componentCount();
        if (want <= 1 && !comps.empty()) {
            return comps[0];
        }
        while (static_cast<int>(comps.size()) < want) {
            comps.push_back(comps.empty() ? b.constF32(0) : comps.back());
        }
        uint32_t id = b.nextId();
        std::vector<uint32_t> ops;
        ops.push_back(b.typeVec(want));
        ops.push_back(id);
        for (int i = 0; i < want; i++) {
            ops.push_back(comps[static_cast<size_t>(i)]);
        }
        b.emit(b.code, kOpCompositeConstruct, ops.data(), static_cast<uint16_t>(ops.size()));
        return note(id, b.typeVec(want));
    }
    case kExprMember: {
        if (!expr->kids.empty() && expr->kids[0]->kind == kExprIdent) {
            const std::string field = expr->kids[0]->name + "." + expr->name;
            if (valueOverlay.count(field) || locals.count(field)) {
                return loadIdent(field);
            }
        }
        uint32_t base = emitExpr(expr->kids[0].get());
        const std::string &sw = expr->name;
        if (isMat(base) || (!sw.empty() && sw[0] == '_')) {
            return extractMatrix(base, sw);
        }
        if (!isVec(base)) {
            return base;
        }
        if (sw.size() == 1) {
            uint32_t id = b.nextId();
            b.emit4(b.code, kOpCompositeExtract, b.typeFloat(), id, base, static_cast<uint32_t>(swizzleIndex(sw[0])));
            return note(id, b.typeFloat());
        }
        uint32_t id = b.nextId();
        std::vector<uint32_t> ops;
        ops.push_back(b.typeVec(static_cast<int>(sw.size())));
        ops.push_back(id);
        ops.push_back(base);
        ops.push_back(base);
        for (size_t i = 0; i < sw.size(); i++) {
            ops.push_back(static_cast<uint32_t>(swizzleIndex(sw[i])));
        }
        b.emit(b.code, kOpVectorShuffle, ops.data(), static_cast<uint16_t>(ops.size()));
        return note(id, b.typeVec(static_cast<int>(sw.size())));
    }
    case kExprUnary: {
        uint32_t x = emitExpr(expr->kids[0].get());
        if (expr->op == "++" || expr->op == "--") {
            uint32_t ty = valueTypes.count(x) ? valueTypes[x] : b.typeFloat();
            uint32_t one = (ty == b.typeInt()) ? b.constI32(1) : b.constF32(1);
            uint32_t id = b.nextId();
            b.emit4(b.code, (ty == b.typeInt()) ? (expr->op == "++" ? kOpIAdd : kOpISub)
                                                : (expr->op == "++" ? kOpFAdd : kOpFSub),
                ty, id, x, one);
            note(id, ty);
            if (!expr->kids.empty() && expr->kids[0]->kind == kExprIdent) {
                auto it = locals.find(expr->kids[0]->name);
                if (it != locals.end()) {
                    b.emit2(b.code, kOpStore, it->second, id);
                }
            }
            return id;
        }
        uint32_t id = b.nextId();
        if (expr->op == "-") {
            uint32_t ty = valueTypes.count(x) ? valueTypes[x] : b.typeFloat();
            b.emit3(b.code, kOpFNegate, ty, id, x);
            return note(id, ty);
        }
        if (expr->op == "!") {
            b.emit3(b.code, kOpLogicalNot, b.typeBool(), id, asBool(x));
            return note(id, b.typeBool());
        }
        return x;
    }
    case kExprBinary: {
        uint32_t l = emitExpr(expr->kids[0].get());
        uint32_t r = emitExpr(expr->kids[1].get());
        uint32_t id = b.nextId();
        uint16_t op = kOpFAdd;
        uint32_t ty = b.typeFloat();
        const bool ints = valueTypes.count(l) && valueTypes[l] == b.typeInt() && valueTypes.count(r) &&
            valueTypes[r] == b.typeInt();
        if (!ints && valueTypes.count(l) && valueTypes[l] == b.typeInt() &&
            !(valueTypes.count(r) && valueTypes[r] == b.typeInt())) {
            uint32_t conv = b.nextId();
            b.emit3(b.code, kOpConvertSToF, b.typeFloat(), conv, l);
            l = note(conv, b.typeFloat());
        }
        if (!ints && valueTypes.count(r) && valueTypes[r] == b.typeInt() &&
            !(valueTypes.count(l) && valueTypes[l] == b.typeInt())) {
            uint32_t conv = b.nextId();
            b.emit3(b.code, kOpConvertSToF, b.typeFloat(), conv, r);
            r = note(conv, b.typeFloat());
        }
        if (expr->op == "+") {
            op = ints ? kOpIAdd : kOpFAdd;
            ty = ints ? b.typeInt() : b.typeFloat();
        }
        else if (expr->op == "-") {
            op = ints ? kOpISub : kOpFSub;
            ty = ints ? b.typeInt() : b.typeFloat();
        }
        else if (expr->op == "*") {
            if (isVec(l) && !isVec(r)) {
                uint32_t idv = b.nextId();
                uint32_t ty = valueTypes[l];
                b.emit4(b.code, kOpVectorTimesScalar, ty, idv, l, r);
                return note(idv, ty);
            }
            if (!isVec(l) && isVec(r)) {
                uint32_t idv = b.nextId();
                uint32_t ty = valueTypes[r];
                b.emit4(b.code, kOpVectorTimesScalar, ty, idv, r, l);
                return note(idv, ty);
            }
            op = kOpFMul;
        }
        else if (expr->op == "/") {
            op = kOpFDiv;
        }
        else if (expr->op == "%") {
            op = kOpFMod;
        }
        else if (expr->op == "<") {
            op = ints ? kOpSLessThan : kOpFOrdLessThan;
            ty = b.typeBool();
        }
        else if (expr->op == ">") {
            op = ints ? kOpSGreaterThan : kOpFOrdGreaterThan;
            ty = b.typeBool();
        }
        else if (expr->op == "<=") {
            op = ints ? kOpSLessThanEqual : kOpFOrdLessThanEqual;
            ty = b.typeBool();
        }
        else if (expr->op == ">=") {
            op = ints ? kOpSGreaterThanEqual : kOpFOrdGreaterThanEqual;
            ty = b.typeBool();
        }
        else if (expr->op == "==") {
            op = ints ? kOpIEqual : kOpFOrdEqual;
            ty = b.typeBool();
        }
        else if (expr->op == "!=") {
            op = kOpFOrdNotEqual;
            ty = b.typeBool();
        }
        else if (expr->op == "=") {
            if (expr->kids[0]->kind == kExprIndex) {
                uint32_t elemTy = 0;
                uint32_t ptr = emitIndexPtr(expr->kids[0].get(), elemTy);
                if (ptr) {
                    b.emit2(b.code, kOpStore, ptr, r);
                }
                return r;
            }
            if (expr->kids[0]->kind == kExprMember && !expr->kids[0]->kids.empty() &&
                expr->kids[0]->kids[0]->kind == kExprIdent) {
                const std::string field = expr->kids[0]->kids[0]->name + "." + expr->kids[0]->name;
                auto fit = locals.find(field);
                if (fit != locals.end()) {
                    b.emit2(b.code, kOpStore, fit->second, r);
                    return r;
                }
                const std::string &mname = expr->kids[0]->kids[0]->name;
                auto it = locals.find(mname);
                if (it != locals.end()) {
                    uint32_t loaded = loadIdent(mname);
                    uint32_t inserted = insertMatrix(loaded, expr->kids[0]->name, r);
                    b.emit2(b.code, kOpStore, it->second, inserted);
                }
                return r;
            }
            auto it = locals.find(expr->kids[0]->name);
            if (it != locals.end()) {
                uint32_t destTy = localTypes.count(expr->kids[0]->name) ? localTypes[expr->kids[0]->name] : 0;
                if ((destTy == b.idMat3 || destTy == b.idMat4) && !isMat(r)) {
                    r = splatMatrix(r, destTy, destTy == b.idMat3 ? 3 : 4);
                }
                b.emit2(b.code, kOpStore, it->second, r);
            }
            return r;
        }
        b.emit4(b.code, op, ty, id, l, r);
        return id;
    }
    case kExprCall: {
        const std::string &name = expr->name;
        if (name == "tex2D" || name == "tex2Dlod" || name == "tex2Dproj" || name == "tex2Dbias" || name == "texCUBE" ||
            name == "tex3D" || name == "Sample" || name == "SampleLevel") {
            std::string sampName;
            if (expr->kids.size() > 1 && expr->kids[1]->kind == kExprIdent) {
                sampName = expr->kids[1]->name;
            }
            uint32_t uv = expr->kids.size() > 2 ? emitExpr(expr->kids[2].get()) : b.constVec4(0, 0, 0, 0);
            uint32_t lod = 0;
            if (name == "SampleLevel" && expr->kids.size() > 3) {
                lod = emitExpr(expr->kids[3].get());
            }
            else if (name == "tex2Dlod" && isVec(uv) && valueTypes[uv] == b.typeVec(4)) {
                lod = extractComp(uv, 3);
            }
            return sampleTexture(sampName, uv, lod);
        }
        if (name.compare(0, 7, "texM3x3") == 0) {
            std::string sampName;
            if (expr->kids.size() > 1 && expr->kids[1]->kind == kExprIdent) {
                sampName = expr->kids[1]->name;
            }
            uint32_t t0 = expr->kids.size() > 2 ? emitExpr(expr->kids[2].get()) : b.constF32(0);
            uint32_t t1 = expr->kids.size() > 3 ? emitExpr(expr->kids[3].get()) : b.constF32(0);
            uint32_t t2 = expr->kids.size() > 4 ? emitExpr(expr->kids[4].get()) : b.constF32(0);
            uint32_t dir = makeVec3(extractComp(t0, 2), extractComp(t1, 2), extractComp(t2, 2));
            if (name == "texM3x3vspec" && b.idGlsl) {
                uint32_t nrm = b.extInst(b.typeVec(3), kGlslNormalize, &dir, 1);
                note(nrm, b.typeVec(3));
                uint32_t view = expr->kids.size() > 5 ? emitExpr(expr->kids[5].get()) : makeVec3(b.constF32(0), b.constF32(0), b.constF32(1));
                uint32_t neg = b.nextId();
                b.emit3(b.code, kOpFNegate, b.typeVec(3), neg, view);
                note(neg, b.typeVec(3));
                uint32_t refs[2] = { neg, nrm };
                uint32_t r = b.extInst(b.typeVec(3), kGlslReflect, refs, 2);
                note(r, b.typeVec(3));
                dir = r;
            }
            uint32_t uv = makeVec2(extractComp(dir, 0), extractComp(dir, 1));
            uint32_t half = b.constF32(0.5f);
            uint32_t scaled = b.nextId();
            b.emit4(b.code, kOpVectorTimesScalar, b.typeVec(2), scaled, uv, half);
            note(scaled, b.typeVec(2));
            uint32_t half2 = makeVec2(half, half);
            uint32_t biased = b.nextId();
            b.emit4(b.code, kOpFAdd, b.typeVec(2), biased, scaled, half2);
            note(biased, b.typeVec(2));
            return sampleTexture(sampName, biased);
        }
        if (name == "saturate" && b.idGlsl) {
            uint32_t x = emitExpr(expr->kids.size() > 1 ? expr->kids[1].get() : nullptr);
            uint32_t ty = valueTypes.count(x) ? valueTypes[x] : b.typeFloat();
            uint32_t zero = b.constF32(0);
            uint32_t one = b.constF32(1);
            uint32_t args[3] = { x, zero, one };
            uint32_t id = b.extInst(ty, kGlslFClamp, args, 3);
            return note(id, ty);
        }
        if ((name == "lerp" || name == "mix") && expr->kids.size() >= 4 && b.idGlsl) {
            uint32_t a = emitExpr(expr->kids[1].get());
            uint32_t c = emitExpr(expr->kids[2].get());
            uint32_t t = emitExpr(expr->kids[3].get());
            uint32_t ty = valueTypes.count(a) ? valueTypes[a] : b.typeFloat();
            uint32_t args[3] = { a, c, t };
            uint32_t id = b.extInst(ty, kGlslFMix, args, 3);
            return note(id, ty);
        }
        if (name == "normalize" && b.idGlsl) {
            uint32_t x = emitExpr(expr->kids.size() > 1 ? expr->kids[1].get() : nullptr);
            uint32_t ty = valueTypes.count(x) ? valueTypes[x] : b.typeVec(3);
            uint32_t id = b.extInst(ty, kGlslNormalize, &x, 1);
            return note(id, ty);
        }
        if (name == "pow" && b.idGlsl) {
            uint32_t x = emitExpr(expr->kids.size() > 1 ? expr->kids[1].get() : nullptr);
            uint32_t y = emitExpr(expr->kids.size() > 2 ? expr->kids[2].get() : nullptr);
            uint32_t ty = valueTypes.count(x) ? valueTypes[x] : b.typeFloat();
            uint32_t args[2] = { x, y };
            uint32_t id = b.extInst(ty, kGlslPow, args, 2);
            return note(id, ty);
        }
        if (name == "exp" && b.idGlsl) {
            uint32_t x = emitExpr(expr->kids.size() > 1 ? expr->kids[1].get() : nullptr);
            uint32_t ty = valueTypes.count(x) ? valueTypes[x] : b.typeFloat();
            uint32_t id = b.extInst(ty, kGlslExp, &x, 1);
            return note(id, ty);
        }
        if ((name == "sin" || name == "cos") && b.idGlsl) {
            uint32_t x = emitExpr(expr->kids.size() > 1 ? expr->kids[1].get() : nullptr);
            uint32_t ty = valueTypes.count(x) ? valueTypes[x] : b.typeFloat();
            uint32_t id = b.extInst(ty, name == "sin" ? kGlslSin : kGlslCos, &x, 1);
            return note(id, ty);
        }
        if (name == "floor" && b.idGlsl) {
            uint32_t x = emitExpr(expr->kids.size() > 1 ? expr->kids[1].get() : nullptr);
            uint32_t ty = valueTypes.count(x) ? valueTypes[x] : b.typeFloat();
            uint32_t id = b.extInst(ty, kGlslFloor, &x, 1);
            return note(id, ty);
        }
        if (name == "abs" && b.idGlsl) {
            uint32_t x = emitExpr(expr->kids.size() > 1 ? expr->kids[1].get() : nullptr);
            uint32_t ty = valueTypes.count(x) ? valueTypes[x] : b.typeFloat();
            uint32_t id = b.extInst(ty, kGlslFAbs, &x, 1);
            return note(id, ty);
        }
        if ((name == "max" || name == "min") && expr->kids.size() >= 3 && b.idGlsl) {
            uint32_t a = emitExpr(expr->kids[1].get());
            uint32_t c = emitExpr(expr->kids[2].get());
            uint32_t ty = valueTypes.count(a) ? valueTypes[a] : b.typeFloat();
            uint32_t args[2] = { a, c };
            uint32_t id = b.extInst(ty, name == "max" ? kGlslFMax : kGlslFMin, args, 2);
            return note(id, ty);
        }
        if (name == "clamp" && expr->kids.size() >= 4 && b.idGlsl) {
            uint32_t x = emitExpr(expr->kids[1].get());
            uint32_t lo = emitExpr(expr->kids[2].get());
            uint32_t hi = emitExpr(expr->kids[3].get());
            uint32_t ty = valueTypes.count(x) ? valueTypes[x] : b.typeFloat();
            uint32_t args[3] = { x, lo, hi };
            uint32_t id = b.extInst(ty, kGlslFClamp, args, 3);
            return note(id, ty);
        }
        if (name == "step" && expr->kids.size() >= 3 && b.idGlsl) {
            uint32_t edge = emitExpr(expr->kids[1].get());
            uint32_t x = emitExpr(expr->kids[2].get());
            uint32_t ty = valueTypes.count(x) ? valueTypes[x] : b.typeFloat();
            uint32_t args[2] = { edge, x };
            uint32_t id = b.extInst(ty, kGlslStep, args, 2);
            return note(id, ty);
        }
        if ((name == "sqrt" || name == "rsqrt") && b.idGlsl) {
            uint32_t x = emitExpr(expr->kids.size() > 1 ? expr->kids[1].get() : nullptr);
            uint32_t ty = valueTypes.count(x) ? valueTypes[x] : b.typeFloat();
            uint32_t id = b.extInst(ty, name == "sqrt" ? kGlslSqrt : kGlslInverseSqrt, &x, 1);
            return note(id, ty);
        }
        if ((name == "frac" || name == "fract") && b.idGlsl) {
            uint32_t x = emitExpr(expr->kids.size() > 1 ? expr->kids[1].get() : nullptr);
            uint32_t ty = valueTypes.count(x) ? valueTypes[x] : b.typeFloat();
            uint32_t id = b.extInst(ty, kGlslFract, &x, 1);
            return note(id, ty);
        }
        if (name == "length" && b.idGlsl) {
            uint32_t x = emitExpr(expr->kids.size() > 1 ? expr->kids[1].get() : nullptr);
            uint32_t id = b.extInst(b.typeFloat(), kGlslLength, &x, 1);
            return note(id, b.typeFloat());
        }
        if (name == "cross" && expr->kids.size() >= 3 && b.idGlsl) {
            uint32_t a = emitExpr(expr->kids[1].get());
            uint32_t c = emitExpr(expr->kids[2].get());
            uint32_t args[2] = { a, c };
            uint32_t id = b.extInst(b.typeVec(3), kGlslCross, args, 2);
            return note(id, b.typeVec(3));
        }
        if (name == "mul") {
            if (expr->kids.size() >= 3) {
                uint32_t l = emitExpr(expr->kids[1].get());
                uint32_t r = emitExpr(expr->kids[2].get());
                uint32_t lty = valueTypes.count(l) ? valueTypes[l] : 0;
                uint32_t rty = valueTypes.count(r) ? valueTypes[r] : 0;
                uint32_t id = b.nextId();
                if (lty && lty == b.idMat4 && rty == b.idMat4) {
                    b.emit4(b.code, kOpMatrixTimesMatrix, b.idMat4, id, l, r);
                    return note(id, b.idMat4);
                }
                if (lty && (lty == b.idMat4 || lty == b.idMat3) && rty && rty != b.idFloat &&
                    rty != b.idMat3 && rty != b.idMat4) {
                    uint32_t oty = (rty == b.typeVec(4) || lty == b.idMat4) ? b.typeVec(4) : b.typeVec(3);
                    b.emit4(b.code, kOpMatrixTimesVector, oty, id, l, r);
                    return note(id, oty);
                }
                if (rty && (rty == b.idMat4 || rty == b.idMat3) && lty && lty != b.idFloat &&
                    lty != b.idMat3 && lty != b.idMat4) {
                    uint32_t oty = (lty == b.typeVec(4) || rty == b.idMat4) ? b.typeVec(4) : b.typeVec(3);
                    b.emit4(b.code, kOpVectorTimesMatrix, oty, id, l, r);
                    return note(id, oty);
                }
                uint32_t ty = lty ? lty : b.typeFloat();
                b.emit4(b.code, kOpFMul, ty, id, l, r);
                return note(id, ty);
            }
        }
        if (name == "dot") {
            uint32_t l = emitExpr(expr->kids.size() > 1 ? expr->kids[1].get() : nullptr);
            uint32_t r = emitExpr(expr->kids.size() > 2 ? expr->kids[2].get() : nullptr);
            uint32_t id = b.nextId();
            b.emit4(b.code, kOpDot, b.typeFloat(), id, l, r);
            return note(id, b.typeFloat());
        }
        uint32_t user = callUser(name, expr);
        if (user) {
            return user;
        }
        if (name == "float4" || name == "float3" || name == "float2") {
            std::unique_ptr<Expr> tmp = Expr::make(kExprConstruct);
            tmp->type = expr->type.kind == kTypeVoid ? Type::vectorType(kTypeFloat, name[5] - '0') : expr->type;
            for (size_t i = 1; i < expr->kids.size(); i++) {
                tmp->kids.push_back(Expr::make(kExprIdent));
            }
            std::vector<uint32_t> comps;
            for (size_t i = 1; i < expr->kids.size(); i++) {
                comps.push_back(emitExpr(expr->kids[i].get()));
            }
            int n = name[5] - '0';
            while (static_cast<int>(comps.size()) < n) {
                comps.push_back(b.constF32(0));
            }
            uint32_t id = b.nextId();
            std::vector<uint32_t> ops;
            ops.push_back(b.typeVec(n));
            ops.push_back(id);
            for (int i = 0; i < n; i++) {
                ops.push_back(comps[static_cast<size_t>(i)]);
            }
            b.emit(b.code, kOpCompositeConstruct, ops.data(), static_cast<uint16_t>(ops.size()));
            return id;
        }
        if (!expr->kids.empty() && expr->kids[0]) {
            return emitExpr(expr->kids.size() > 1 ? expr->kids[1].get() : expr->kids[0].get());
        }
        return b.constF32(0);
    }
    case kExprTernary: {
        uint32_t cond = asBool(emitExpr(expr->kids[0].get()));
        uint32_t t = emitExpr(expr->kids.size() > 1 ? expr->kids[1].get() : nullptr);
        uint32_t f = emitExpr(expr->kids.size() > 2 ? expr->kids[2].get() : nullptr);
        uint32_t ty = valueTypes.count(t) ? valueTypes[t] : b.typeFloat();
        uint32_t id = b.nextId();
        uint32_t ops[5] = { ty, id, cond, t, f };
        b.emit(b.code, kOpSelect, ops, 5);
        return note(id, ty);
    }
    case kExprCast: {
        uint32_t x = emitExpr(expr->kids.empty() ? nullptr : expr->kids[0].get());
        if (expr->type.kind == kTypeStruct) {
            return x;
        }
        uint32_t srcTy = valueTypes.count(x) ? valueTypes[x] : 0;
        if ((expr->type.kind == kTypeFloat || expr->type.kind == kTypeVector) && srcTy == b.typeInt()) {
            uint32_t dst = expr->type.kind == kTypeVector ? b.typeVec(expr->type.rows) : b.typeFloat();
            uint32_t id = b.nextId();
            b.emit3(b.code, kOpConvertSToF, dst, id, x);
            return note(id, dst);
        }
        if (expr->type.kind == kTypeInt && srcTy == b.typeFloat()) {
            uint32_t id = b.nextId();
            b.emit3(b.code, kOpConvertFToS, b.typeInt(), id, x);
            return note(id, b.typeInt());
        }
        return x;
    }
    case kExprIndex: {
        if (!expr->kids.empty() && expr->kids[0]->kind == kExprIdent &&
            uniformTypes.count(expr->kids[0]->name) && uniformTypes[expr->kids[0]->name].kind == kTypeArray) {
            uint32_t index = expr->kids.size() > 1 ? asInt(emitExpr(expr->kids[1].get())) : b.constI32(0);
            return loadUniformElement(expr->kids[0]->name, index);
        }
        uint32_t elemTy = 0;
        uint32_t ptr = emitIndexPtr(expr, elemTy);
        if (ptr) {
            uint32_t id = b.nextId();
            b.emit3(b.code, kOpLoad, elemTy, id, ptr);
            return note(id, elemTy);
        }
        uint32_t base = emitExpr(expr->kids.empty() ? nullptr : expr->kids[0].get());
        if (isVec(base) && expr->kids.size() > 1 && expr->kids[1]->kind == kExprLiteralInt) {
            return extractComp(base, static_cast<uint32_t>(expr->kids[1]->intValue));
        }
        if (isVec(base) && expr->kids.size() > 1) {
            uint32_t idx = asInt(emitExpr(expr->kids[1].get()));
            uint32_t id = b.nextId();
            b.emit4(b.code, kOpVectorExtractDynamic, b.typeFloat(), id, base, idx);
            return note(id, b.typeFloat());
        }
        return base;
    }
    default:
        return b.constF32(0);
    }
}

bool
Emitter::emitStmt(const Stmt *stmt, uint32_t returnType)
{
    if (!stmt) {
        return true;
    }
    switch (stmt->kind) {
    case kStmtBlock:
        for (size_t i = 0; i < stmt->kids.size(); i++) {
            if (!emitStmt(stmt->kids[i].get(), returnType)) {
                return false;
            }
        }
        return true;
    case kStmtReturn: {
        if (stmt->expr && stmt->expr->kind == kExprIdent) {
            const std::string prefix = stmt->expr->name + ".";
            for (auto it = locals.begin(); it != locals.end(); ++it) {
                if (it->first.compare(0, prefix.size(), prefix) == 0) {
                    pendingStructReturn = stmt->expr->name;
                    break;
                }
            }
        }
        if (pendingStructReturn.empty()) {
            pendingReturn = emitExpr(stmt->expr.get());
        }
        return true;
    }
    case kStmtDiscard:
        b.emit(b.code, kOpKill, nullptr, 0);
        return true;
    case kStmtVar: {
        const uint32_t storage =
            stmt->varType.kind == kTypeArray ? kStoragePrivate : kStorageFunction;
        uint32_t ty = b.typeOf(stmt->varType);
        uint32_t ptr = b.ptrType(ty, storage);
        uint32_t var = b.nextId();
        if (storage == kStoragePrivate) {
            b.emit3(b.types, kOpVariable, ptr, var, storage);
        }
        else {
            b.emit3(b.code, kOpVariable, ptr, var, storage);
        }
        locals[stmt->name] = var;
        localTypes[stmt->name] = ty;
        rememberArray(stmt->name, stmt->varType, storage);
        const Type *st = resolveStruct(stmt->varType);
        if (st && !st->members.empty()) {
            allocStructLocals(stmt->name, *st);
            return true;
        }
        if (stmt->expr) {
            if ((ty == b.idMat3 || ty == b.idMat4) && stmt->expr->kind != kExprConstruct) {
                uint32_t scalar = emitExpr(stmt->expr.get());
                if (!isMat(scalar)) {
                    scalar = splatMatrix(scalar, ty, ty == b.idMat3 ? 3 : 4);
                }
                b.emit2(b.code, kOpStore, var, scalar);
            }
            else if (stmt->varType.kind == kTypeArray && stmt->expr->kind == kExprConstruct) {
                std::vector<uint32_t> comps;
                for (size_t i = 0; i < stmt->expr->kids.size(); i++) {
                    comps.push_back(emitExpr(stmt->expr->kids[i].get()));
                }
                int n = stmt->varType.arraySize > 0 ? stmt->varType.arraySize : static_cast<int>(comps.size());
                while (static_cast<int>(comps.size()) < n) {
                    comps.push_back(b.constF32(0));
                }
                uint32_t id = b.nextId();
                std::vector<uint32_t> ops;
                ops.push_back(ty);
                ops.push_back(id);
                for (int i = 0; i < n; i++) {
                    ops.push_back(comps[static_cast<size_t>(i)]);
                }
                b.emit(b.code, kOpCompositeConstruct, ops.data(), static_cast<uint16_t>(ops.size()));
                b.emit2(b.code, kOpStore, var, id);
            }
            else {
                b.emit2(b.code, kOpStore, var, emitExpr(stmt->expr.get()));
            }
        }
        return true;
    }
    case kStmtExpr:
        emitExpr(stmt->expr.get());
        return true;
    case kStmtIf:
        emitIf(stmt, returnType);
        return true;
    case kStmtFor:
        emitFor(stmt, returnType);
        return true;
    case kStmtWhile:
        emitWhile(stmt, returnType, false);
        return true;
    case kStmtDoWhile:
        emitWhile(stmt, returnType, true);
        return true;
    default:
        return true;
    }
}

} /* namespace anonymous */

namespace {

std::unique_ptr<Expr>
makeExpression(const ShaderExpressionIR *source)
{
    if (!source) {
        return std::unique_ptr<Expr>();
    }
    std::unique_ptr<Expr> result = Expr::make(static_cast<ExprKind>(source->kind));
    result->type = source->type;
    result->name = source->name;
    result->op = source->operation;
    result->floatValue = source->floatValue;
    result->intValue = source->intValue;
    result->boolValue = source->boolValue;
    for (std::vector<std::unique_ptr<ShaderExpressionIR> >::const_iterator it = source->children.begin();
         it != source->children.end(); ++it) {
        result->kids.push_back(makeExpression(it->get()));
    }
    return result;
}

std::unique_ptr<Stmt>
makeStatement(const ShaderStatementIR *source)
{
    if (!source) {
        return std::unique_ptr<Stmt>();
    }
    std::unique_ptr<Stmt> result(new Stmt());
    result->kind = static_cast<StmtKind>(source->kind);
    result->varType = source->variableType;
    result->name = source->name;
    result->semantic = source->semantic;
    result->expr = makeExpression(source->expression.get());
    result->expr2 = makeExpression(source->condition.get());
    result->expr3 = makeExpression(source->iteration.get());
    result->thenStmt = makeStatement(source->thenStatement.get());
    result->elseStmt = makeStatement(source->elseStatement.get());
    for (std::vector<std::unique_ptr<ShaderStatementIR> >::const_iterator it = source->children.begin();
         it != source->children.end(); ++it) {
        result->kids.push_back(makeStatement(it->get()));
    }
    return result;
}

Function
makeFunction(const ShaderFunctionIR &source)
{
    Function result;
    result.name = source.name;
    result.returnType = source.returnType;
    result.returnSemantic = source.returnSemantic;
    for (std::vector<ShaderParameterIR>::const_iterator it = source.parameters.begin(); it != source.parameters.end(); ++it) {
        Parameter parameter;
        parameter.name = it->name;
        parameter.type = it->type;
        parameter.semantic = it->semantic;
        parameter.isOut = it->output;
        result.params.push_back(parameter);
    }
    result.body = makeStatement(source.body.get());
    return result;
}

} /* namespace anonymous */

bool
emitFunctionSPIRVWithEffect(const TranslationUnit &unit, const EffectModuleIR *effect, const ShaderModuleIR *shader,
    const Function &fn, SpirvShaderStage stage, std::vector<uint32_t> &words, std::string &error)
{
    Emitter e;
    e.unit = &unit;
    e.effect = effect;
    e.shader = shader;
    if (shader) {
        for (std::vector<ShaderStructIR>::const_iterator it = shader->structs.begin(); it != shader->structs.end(); ++it) {
            Type type;
            type.kind = kTypeStruct;
            type.name = it->name;
            type.members = it->members;
            e.structTypes[type.name] = type;
        }
    }
    e.stage = stage;
    e.b.emit1(e.b.header, kOpCapability, kCapShader);
    e.b.idGlsl = e.b.nextId();
    e.b.emitLiteralString(e.b.header, kOpExtInstImport, e.b.idGlsl, "GLSL.std.450");
    e.b.emit2(e.b.header, kOpMemoryModel, kAddrLogical, kMemGLSL450);

    uint32_t retType = e.b.typeOf(fn.returnType.kind == kTypeVoid ? Type::vectorType(kTypeFloat, 4) : fn.returnType);
    if (fn.returnType.kind == kTypeFloat || fn.returnType.kind == kTypeInt || fn.returnType.kind == kTypeBool) {
        retType = e.b.typeOf(fn.returnType);
    }
    else if (fn.returnType.kind == kTypeVector) {
        retType = e.b.typeVec(fn.returnType.rows);
    }
    else {
        retType = e.b.typeVec(4);
    }

    std::vector<uint32_t> inVars;
    std::vector<uint32_t> extraOuts;
    std::vector<uint32_t> inTypes;
    for (size_t i = 0; i < fn.params.size(); i++) {
        const Type *st = e.resolveStruct(fn.params[i].type);
        if (st && !st->members.empty()) {
            for (size_t mi = 0; mi < st->members.size(); mi++) {
                uint32_t ty = e.b.typeOf(st->members[mi].second);
                const uint32_t storage = fn.params[i].isOut ? kStorageOutput : kStorageInput;
                uint32_t ptr = e.b.ptrType(ty, storage);
                uint32_t var = e.b.nextId();
                e.b.emit3(e.b.types, kOpVariable, ptr, var, storage);
                const std::string &sem = st->members[mi].second.name;
                e.b.decorate(var, kDecorationLocation, static_cast<uint32_t>(semanticLocation(sem)), true);
                if (fn.params[i].isOut) {
                    extraOuts.push_back(var);
                }
                else {
                    inVars.push_back(var);
                }
                const std::string field = fn.params[i].name + "." + st->members[mi].first;
                e.locals[field] = var;
                e.localTypes[field] = ty;
            }
            continue;
        }
        uint32_t ty = e.b.typeOf(fn.params[i].type.kind == kTypeVector || fn.params[i].type.kind == kTypeFloat ||
                fn.params[i].type.kind == kTypeInt
            ? fn.params[i].type
            : Type::vectorType(kTypeFloat, 4));
        if (fn.params[i].type.kind != kTypeVector && fn.params[i].type.kind != kTypeFloat &&
            fn.params[i].type.kind != kTypeInt) {
            ty = e.b.typeVec(4);
        }
        const uint32_t storage = fn.params[i].isOut ? kStorageOutput : kStorageInput;
        uint32_t ptr = e.b.ptrType(ty, storage);
        uint32_t var = e.b.nextId();
        e.b.emit3(e.b.types, kOpVariable, ptr, var, storage);
        e.b.decorate(var, kDecorationLocation, static_cast<uint32_t>(semanticLocation(fn.params[i].semantic)), true);
        if (fn.params[i].isOut) {
            extraOuts.push_back(var);
        }
        else {
            inVars.push_back(var);
        }
        inTypes.push_back(ty);
        e.locals[fn.params[i].name] = var;
        e.localTypes[fn.params[i].name] = ty;
    }

    uint32_t outTy = retType;
    uint32_t outVar = 0;
    const Type *retStruct = e.resolveStruct(fn.returnType);
    if (retStruct && !retStruct->members.empty()) {
        for (size_t mi = 0; mi < retStruct->members.size(); mi++) {
            uint32_t ty = e.b.typeOf(retStruct->members[mi].second);
            uint32_t ptr = e.b.ptrType(ty, kStorageOutput);
            uint32_t var = e.b.nextId();
            e.b.emit3(e.b.types, kOpVariable, ptr, var, kStorageOutput);
            const std::string &sem = retStruct->members[mi].second.name;
            if (stage == kStageVertex && isPositionSemantic(sem)) {
                e.b.decorate(var, kDecorationBuiltIn, kBuiltInPosition, true);
                outVar = var;
                outTy = ty;
            }
            else {
                e.b.decorate(var, kDecorationLocation, static_cast<uint32_t>(semanticLocation(sem)), true);
                extraOuts.push_back(var);
            }
            e.structOutVars[retStruct->members[mi].first] = var;
        }
        if (!outVar) {
            outVar = extraOuts.empty() ? 0 : extraOuts[0];
        }
    }
    if (!outVar) {
        uint32_t outPtr = e.b.ptrType(outTy, kStorageOutput);
        outVar = e.b.nextId();
        e.b.emit3(e.b.types, kOpVariable, outPtr, outVar, kStorageOutput);
        if (stage == kStageVertex && isPositionSemantic(fn.returnSemantic)) {
            e.b.decorate(outVar, kDecorationBuiltIn, kBuiltInPosition, true);
        }
        else {
            e.b.decorate(outVar, kDecorationLocation, 0, true);
        }
    }

    int samplerIndex = 0;
    for (size_t i = 0; i < unit.variables.size(); i++) {
        if (!unit.variables[i].type.isSampler()) {
            continue;
        }
        uint32_t imageType = e.b.nextId();
        uint32_t sampledType = e.b.nextId();
        uint32_t dim = kDim2D;
        if (unit.variables[i].type.samplerDim == kSamplerCube) {
            dim = kDimCube;
        }
        else if (unit.variables[i].type.samplerDim == kSampler3D) {
            dim = kDim3D;
        }
        uint32_t ops[8] = { imageType, e.b.typeFloat(), dim, 0, 0, 0, 1, 0 };
        e.b.emit(e.b.types, kOpTypeImage, ops, 8);
        e.b.emit2(e.b.types, kOpTypeSampledImage, sampledType, imageType);
        uint32_t ptr = e.b.ptrType(sampledType, kStorageUniformConstant);
        uint32_t var = e.b.nextId();
        e.b.emit3(e.b.types, kOpVariable, ptr, var, kStorageUniformConstant);
        int binding = samplerIndex;
        const EffectBindingIR *effectBinding = findEffectBinding(e.effect, unit.variables[i].name, kEffectRegisterSampler);
        if (effectBinding) {
            binding = effectBinding->registerIndex;
        }
        else if (!unit.variables[i].registerName.empty() &&
            (unit.variables[i].registerName[0] == 's' || unit.variables[i].registerName[0] == 'S')) {
            binding = std::atoi(unit.variables[i].registerName.c_str() + 1);
        }
        e.b.decorate(var, kDecorationDescriptorSet, 0, true);
        e.b.decorate(var, kDecorationBinding, static_cast<uint32_t>(binding), true);
        e.samplers[unit.variables[i].name] = var;
        e.samplerTypes[unit.variables[i].name] = sampledType;
        e.samplerDims[unit.variables[i].name] = unit.variables[i].type.samplerDim;
        samplerIndex++;
        if (samplerIndex > 32) {
            break;
        }
    }
    const uint32_t uniformCapacity = 128;
    bool hasUniforms = false;
    for (size_t i = 0; i < unit.variables.size(); i++) {
        const Variable &gv = unit.variables[i];
        if (gv.type.isSampler() || gv.type.kind == kTypeTexture || gv.type.kind == kTypeStruct ||
            gv.type.kind == kTypeString || gv.type.kind == kTypeVoid ||
            (gv.type.kind == kTypeArray && gv.type.scalar != kTypeFloat) ||
            gv.type.kind == kTypeInt || gv.type.kind == kTypeUInt || gv.type.kind == kTypeBool) {
            continue;
        }
        const EffectBindingIR *binding = findEffectBinding(e.effect, gv.name, kEffectRegisterFloat4);
        if (!binding) {
            continue;
        }
        e.uniformRegisters[gv.name] = static_cast<uint32_t>(binding->registerIndex);
        e.uniformTypes[gv.name] = gv.type;
        hasUniforms = true;
    }
    if (hasUniforms) {
        const uint32_t vec4 = e.b.typeVec(4);
        const uint32_t length = e.b.constU32(uniformCapacity);
        const uint32_t array = e.b.nextId();
        e.b.emit3(e.b.types, kOpTypeArray, array, vec4, length);
        e.b.decorate(array, kDecorationArrayStride, 16, true);
        const uint32_t block = e.b.nextId();
        e.b.emit2(e.b.types, kOpTypeStruct, block, array);
        e.b.decorate(block, kDecorationBlock, 0, false);
        e.b.emit4(e.b.decorations, kOpMemberDecorate, block, 0, kDecorationOffset, 0);
        e.b.memberName(block, 0, "data");
        const uint32_t ptr = e.b.ptrType(block, kStorageUniform);
        e.uniformBuffer = e.b.nextId();
        e.b.emit3(e.b.types, kOpVariable, ptr, e.uniformBuffer, kStorageUniform);
        e.b.decorate(e.uniformBuffer, kDecorationDescriptorSet, 0, true);
        e.b.decorate(e.uniformBuffer, kDecorationBinding, 0, true);
        e.b.name(e.uniformBuffer, stage == kStageVertex ? "vs_uniforms_vec4" : "ps_uniforms_vec4");
    }
    e.b.typeFloat();
    e.b.typeVec(4);
    for (size_t i = 0; i < unit.variables.size(); i++) {
        const Variable &gv = unit.variables[i];
        if (gv.type.isSampler() || gv.type.kind == kTypeTexture || gv.type.kind == kTypeStruct ||
            gv.type.kind == kTypeString || gv.type.kind == kTypeVoid || e.uniformRegisters.count(gv.name)) {
            continue;
        }
        if (e.locals.find(gv.name) != e.locals.end()) {
            continue;
        }
        uint32_t ty = e.b.typeOf(gv.type);
        uint32_t ptr = e.b.ptrType(ty, kStoragePrivate);
        uint32_t var = e.b.nextId();
        e.b.emit3(e.b.types, kOpVariable, ptr, var, kStoragePrivate);
        e.locals[gv.name] = var;
        e.localTypes[gv.name] = ty;
        e.rememberArray(gv.name, gv.type, kStoragePrivate);
    }

    uint32_t fnType = e.b.nextId();
    e.b.emit2(e.b.types, kOpTypeFunction, fnType, e.b.typeVoid());
    uint32_t mainId = e.b.nextId();
    uint32_t entryOps[4] = { stage == kStageVertex ? static_cast<uint32_t>(kExecVertex)
                                                   : static_cast<uint32_t>(kExecFragment),
        mainId, 0, 0 };
    std::vector<uint32_t> ep;
    ep.push_back(stage == kStageVertex ? kExecVertex : kExecFragment);
    ep.push_back(mainId);
    const char *entryName = "main";
    uint32_t packed = 0;
    int shift = 0;
    ep.push_back(0);
    size_t nameIndex = ep.size() - 1;
    for (const char *p = entryName;; p++) {
        packed |= (static_cast<uint32_t>(static_cast<unsigned char>(*p)) << shift);
        shift += 8;
        if (shift == 32 || *p == 0) {
            ep[nameIndex] = packed;
            if (*p == 0) {
                break;
            }
            packed = 0;
            shift = 0;
            ep.push_back(0);
            nameIndex = ep.size() - 1;
        }
    }
    for (size_t i = 0; i < inVars.size(); i++) {
        ep.push_back(inVars[i]);
    }
    for (size_t i = 0; i < extraOuts.size(); i++) {
        ep.push_back(extraOuts[i]);
    }
    ep.push_back(outVar);
    e.b.emit(e.b.header, kOpEntryPoint, ep.data(), static_cast<uint16_t>(ep.size()));
    if (stage == kStageFragment) {
        e.b.emit2(e.b.header, kOpExecutionMode, mainId, kExecOriginUpperLeft);
    }

    e.pendingReturn = 0;
    e.b.emit4(e.b.code, kOpFunction, e.b.typeVoid(), mainId, kFunctionControlNone, fnType);
    uint32_t label = e.b.nextId();
    e.b.emit1(e.b.code, kOpLabel, label);

    if (!e.emitStmt(fn.body.get(), retType)) {
        error = e.b.error.empty() ? "emit failed" : e.b.error;
        return false;
    }

    if (!e.pendingStructReturn.empty()) {
        for (auto it = e.structOutVars.begin(); it != e.structOutVars.end(); ++it) {
            const std::string field = e.pendingStructReturn + "." + it->first;
            if (e.locals.count(field)) {
                e.b.emit2(e.b.code, kOpStore, it->second, e.loadIdent(field));
            }
        }
    }
    else {
        uint32_t retValue = e.pendingReturn ? e.pendingReturn : e.b.constVec4(0, 0, 0, 1);
        e.b.emit2(e.b.code, kOpStore, outVar, retValue);
    }
    e.b.emit(e.b.code, kOpReturn, nullptr, 0);
    e.b.emit(e.b.code, kOpFunctionEnd, nullptr, 0);

    words.clear();
    words.push_back(0x07230203);
    words.push_back(0x00010000);
    words.push_back(0);
    words.push_back(e.b.bound);
    words.push_back(0);
    words.insert(words.end(), e.b.header.begin(), e.b.header.end());
    words.insert(words.end(), e.b.debug.begin(), e.b.debug.end());
    words.insert(words.end(), e.b.decorations.begin(), e.b.decorations.end());
    words.insert(words.end(), e.b.types.begin(), e.b.types.end());
    words.insert(words.end(), e.b.code.begin(), e.b.code.end());
    (void) entryOps;
    (void) findFunction;
    size_t cursor = 5;
    while (cursor < words.size()) {
        uint32_t wordCount = words[cursor] >> 16;
        uint32_t opcode = words[cursor] & 0xFFFFu;
        if (wordCount == 0 || cursor + wordCount > words.size()) {
            error = "invalid SPIR-V at word " + std::to_string(cursor) + " op=" + std::to_string(opcode) +
                " count=" + std::to_string(wordCount) + " size=" + std::to_string(words.size());
            return false;
        }
        cursor += wordCount;
    }
    if (cursor != words.size()) {
        error = "SPIR-V trailing words";
        return false;
    }
    return true;
}

bool
emitFunctionSPIRV(
    const TranslationUnit &unit, const Function &fn, SpirvShaderStage stage, std::vector<uint32_t> &words,
    std::string &error)
{
    return emitFunctionSPIRVWithEffect(unit, nullptr, nullptr, fn, stage, words, error);
}

bool
validateSPIRV(const std::vector<uint32_t> &words, std::string &error)
{
    if (words.size() < 5 || words[0] != 0x07230203 || words[3] < 1) {
        error = "invalid SPIR-V header";
        return false;
    }
    bool capability = false, memoryModel = false, entryPoint = false, function = false, functionEnd = false;
    size_t cursor = 5;
    while (cursor < words.size()) {
        const uint32_t wordCount = words[cursor] >> 16;
        const uint32_t opcode = words[cursor] & 0xFFFFu;
        if (wordCount == 0 || cursor + wordCount > words.size()) {
            error = "invalid SPIR-V instruction at word " + std::to_string(cursor) + " op=" +
                std::to_string(opcode);
            return false;
        }
        switch (opcode) {
        case kOpCapability:
            capability = true;
            break;
        case kOpMemoryModel:
            memoryModel = true;
            break;
        case kOpEntryPoint:
            entryPoint = true;
            break;
        case kOpFunction:
            function = true;
            break;
        case kOpFunctionEnd:
            functionEnd = true;
            break;
        case kOpExtInstImport:
        case kOpName:
        case kOpMemberName:
        case kOpExecutionMode:
        case kOpTypeVoid:
        case kOpTypeBool:
        case kOpTypeInt:
        case kOpTypeFloat:
        case kOpTypeVector:
        case kOpTypeMatrix:
        case kOpTypeImage:
        case kOpTypeSampler:
        case kOpTypeSampledImage:
        case kOpTypeArray:
        case kOpTypeRuntimeArray:
        case kOpTypeStruct:
        case kOpTypePointer:
        case kOpTypeFunction:
        case kOpConstantTrue:
        case kOpConstantFalse:
        case kOpConstant:
        case kOpConstantComposite:
        case kOpVariable:
        case kOpLoad:
        case kOpStore:
        case kOpAccessChain:
        case kOpDecorate:
        case kOpMemberDecorate:
        case kOpVectorExtractDynamic:
        case kOpVectorShuffle:
        case kOpCompositeConstruct:
        case kOpCompositeExtract:
        case kOpCompositeInsert:
        case kOpImageSampleImplicitLod:
        case kOpImageSampleExplicitLod:
        case kOpConvertFToS:
        case kOpConvertSToF:
        case kOpFNegate:
        case kOpIAdd:
        case kOpFAdd:
        case kOpISub:
        case kOpFSub:
        case kOpIMul:
        case kOpFMul:
        case kOpUDiv:
        case kOpSDiv:
        case kOpFDiv:
        case kOpSMod:
        case kOpFMod:
        case kOpVectorTimesScalar:
        case kOpVectorTimesMatrix:
        case kOpMatrixTimesVector:
        case kOpMatrixTimesMatrix:
        case kOpDot:
        case kOpSelect:
        case kOpLogicalNot:
        case kOpIEqual:
        case kOpINotEqual:
        case kOpSGreaterThan:
        case kOpSGreaterThanEqual:
        case kOpSLessThan:
        case kOpSLessThanEqual:
        case kOpFOrdEqual:
        case kOpFOrdNotEqual:
        case kOpFOrdLessThan:
        case kOpFOrdGreaterThan:
        case kOpFOrdLessThanEqual:
        case kOpFOrdGreaterThanEqual:
        case kOpPhi:
        case kOpLoopMerge:
        case kOpSelectionMerge:
        case kOpLabel:
        case kOpBranch:
        case kOpBranchConditional:
        case kOpReturn:
        case kOpReturnValue:
        case kOpUnreachable:
        case kOpKill:
        case kOpFunctionParameter:
            break;
        default:
            error = "unknown SPIR-V opcode " + std::to_string(opcode) + " at word " + std::to_string(cursor);
            return false;
        }
        cursor += wordCount;
    }
    if (cursor != words.size()) {
        error = "SPIR-V trailing words";
        return false;
    }
    if (!capability || !memoryModel || !entryPoint || !function || !functionEnd) {
        error = "SPIR-V shader module is missing required instructions";
        return false;
    }
    return true;
}

bool
emitShaderSPIRV(const EffectModuleIR &effect, const ShaderModuleIR &shader, std::vector<uint32_t> &words,
    std::string &error)
{
    if (shader.functions.empty()) {
        error = "shader IR has no entry function";
        return false;
    }
    TranslationUnit semanticUnit;
    for (std::vector<ShaderGlobalIR>::const_iterator it = shader.globals.begin(); it != shader.globals.end(); ++it) {
        Variable variable;
        variable.name = it->name;
        variable.type = it->type;
        variable.semantic = it->semantic;
        variable.registerName = it->registerName;
        variable.textureName = it->textureName;
        semanticUnit.variables.push_back(std::move(variable));
    }
    for (std::vector<ShaderFunctionIR>::const_iterator it = shader.functions.begin(); it != shader.functions.end(); ++it) {
        semanticUnit.functions.push_back(makeFunction(*it));
    }
    return emitFunctionSPIRVWithEffect(semanticUnit, &effect, &shader, semanticUnit.functions[0],
        shader.stage == kShaderStageVertex ? kStageVertex : kStageFragment, words, error);
}

} /* namespace fx9next */

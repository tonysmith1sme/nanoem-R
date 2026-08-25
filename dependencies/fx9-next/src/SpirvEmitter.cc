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
    kOpVectorShuffle = 79,
    kOpCompositeConstruct = 80,
    kOpCompositeExtract = 81,
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
    kOpMatrixTimesVector = 145,
    kOpMatrixTimesMatrix = 146,
    kOpDot = 148,
    kOpLogicalNot = 168,
    kOpIEqual = 170,
    kOpINotEqual = 171,
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
    kAddrLogical = 0,
    kMemGLSL450 = 1,
    kStorageFunction = 7,
    kStorageInput = 1,
    kStorageOutput = 3,
    kStorageUniformConstant = 0,
    kStorageUniform = 2,
    kStoragePrivate = 6,
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

struct Builder {
    std::vector<uint32_t> header;
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
    {
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
        if (t.kind == kTypeMatrix) {
            uint32_t col = typeVec(t.rows);
            uint32_t id = nextId();
            emit3(types, kOpTypeMatrix, id, col, static_cast<uint32_t>(t.columns));
            return id;
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
    ShaderStage stage;
    std::unordered_map<std::string, uint32_t> locals;
    std::unordered_map<std::string, uint32_t> localTypes;
    std::unordered_map<std::string, uint32_t> samplers;
    uint32_t currentFnType;
    uint32_t pendingReturn;

    uint32_t emitExpr(const Expr *expr);
    bool emitStmt(const Stmt *stmt, uint32_t returnType);
    uint32_t loadIdent(const std::string &name);
};

uint32_t
Emitter::loadIdent(const std::string &name)
{
    auto it = locals.find(name);
    if (it == locals.end()) {
        return b.constF32(0);
    }
    uint32_t resultType = localTypes[name];
    uint32_t id = b.nextId();
    b.emit3(b.code, kOpLoad, resultType, id, it->second);
    return id;
}

uint32_t
Emitter::emitExpr(const Expr *expr)
{
    if (!expr) {
        return b.constF32(0);
    }
    switch (expr->kind) {
    case kExprLiteralFloat:
        return b.constF32(static_cast<float>(expr->floatValue));
    case kExprLiteralInt:
        return b.constI32(expr->intValue);
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
        return id;
    }
    case kExprMember: {
        uint32_t base = emitExpr(expr->kids[0].get());
        const std::string &sw = expr->name;
        if (sw.size() == 1) {
            uint32_t id = b.nextId();
            b.emit4(b.code, kOpCompositeExtract, b.typeFloat(), id, base, static_cast<uint32_t>(swizzleIndex(sw[0])));
            return id;
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
        return id;
    }
    case kExprUnary: {
        uint32_t x = emitExpr(expr->kids[0].get());
        uint32_t id = b.nextId();
        if (expr->op == "-") {
            b.emit3(b.code, kOpFNegate, b.typeFloat(), id, x);
        }
        else if (expr->op == "!") {
            b.emit3(b.code, kOpLogicalNot, b.typeBool(), id, x);
        }
        else {
            return x;
        }
        return id;
    }
    case kExprBinary: {
        uint32_t l = emitExpr(expr->kids[0].get());
        uint32_t r = emitExpr(expr->kids[1].get());
        uint32_t id = b.nextId();
        uint16_t op = kOpFAdd;
        uint32_t ty = b.typeFloat();
        if (expr->op == "+") {
            op = kOpFAdd;
        }
        else if (expr->op == "-") {
            op = kOpFSub;
        }
        else if (expr->op == "*") {
            op = kOpFMul;
        }
        else if (expr->op == "/") {
            op = kOpFDiv;
        }
        else if (expr->op == "%") {
            op = kOpFMod;
        }
        else if (expr->op == "<") {
            op = kOpFOrdLessThan;
            ty = b.typeBool();
        }
        else if (expr->op == ">") {
            op = kOpFOrdGreaterThan;
            ty = b.typeBool();
        }
        else if (expr->op == "<=") {
            op = kOpFOrdLessThanEqual;
            ty = b.typeBool();
        }
        else if (expr->op == ">=") {
            op = kOpFOrdGreaterThanEqual;
            ty = b.typeBool();
        }
        else if (expr->op == "==") {
            op = kOpFOrdEqual;
            ty = b.typeBool();
        }
        else if (expr->op == "!=") {
            op = kOpFOrdNotEqual;
            ty = b.typeBool();
        }
        else if (expr->op == "=") {
            auto it = locals.find(expr->kids[0]->name);
            if (it != locals.end()) {
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
            name == "tex3D" || name.compare(0, 7, "texM3x3") == 0) {
            uint32_t uv = expr->kids.size() > 2 ? emitExpr(expr->kids[2].get()) : b.constVec4(0, 0, 0, 0);
            std::string sampName;
            if (expr->kids.size() > 1 && expr->kids[1]->kind == kExprIdent) {
                sampName = expr->kids[1]->name;
            }
            auto sit = samplers.find(sampName);
            uint32_t sampled = 0;
            if (sit != samplers.end()) {
                uint32_t loaded = b.nextId();
                uint32_t sampledType = b.nextId();
                uint32_t imageType = b.nextId();
                uint32_t floatTy = b.typeFloat();
                uint32_t imageOps[8] = { imageType, floatTy, kDim2D, 0, 0, 0, 1, 0 };
                b.emit(b.types, kOpTypeImage, imageOps, 8);
                b.emit2(b.types, kOpTypeSampledImage, sampledType, imageType);
                b.emit3(b.code, kOpLoad, sampledType, loaded, sit->second);
                sampled = loaded;
            }
            else {
                return b.constVec4(0, 0, 0, 1);
            }
            uint32_t coord = uv;
            uint32_t id = b.nextId();
            b.emit4(b.code, kOpImageSampleImplicitLod, b.typeVec(4), id, sampled, coord);
            return id;
        }
        if (name == "saturate") {
            return emitExpr(expr->kids.size() > 1 ? expr->kids[1].get() : nullptr);
        }
        if (name == "lerp" || name == "mix") {
            if (expr->kids.size() >= 4) {
                return emitExpr(expr->kids[1].get());
            }
        }
        if (name == "mul") {
            if (expr->kids.size() >= 3) {
                uint32_t l = emitExpr(expr->kids[1].get());
                uint32_t r = emitExpr(expr->kids[2].get());
                uint32_t id = b.nextId();
                b.emit4(b.code, kOpFMul, b.typeFloat(), id, l, r);
                return id;
            }
        }
        if (name == "dot") {
            uint32_t l = emitExpr(expr->kids.size() > 1 ? expr->kids[1].get() : nullptr);
            uint32_t r = emitExpr(expr->kids.size() > 2 ? expr->kids[2].get() : nullptr);
            uint32_t id = b.nextId();
            b.emit4(b.code, kOpDot, b.typeFloat(), id, l, r);
            return id;
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
        return emitExpr(expr->kids.size() > 1 ? expr->kids[1].get() : nullptr);
    }
    case kExprCast:
        return emitExpr(expr->kids.empty() ? nullptr : expr->kids[0].get());
    case kExprIndex:
        return emitExpr(expr->kids.empty() ? nullptr : expr->kids[0].get());
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
        pendingReturn = emitExpr(stmt->expr.get());
        return true;
    }
    case kStmtDiscard:
        b.emit(b.code, kOpKill, nullptr, 0);
        return true;
    case kStmtVar: {
        uint32_t ty = b.typeOf(stmt->varType);
        uint32_t ptr = b.ptrType(ty, kStorageFunction);
        uint32_t var = b.nextId();
        b.emit3(b.code, kOpVariable, ptr, var, kStorageFunction);
        locals[stmt->name] = var;
        localTypes[stmt->name] = ty;
        if (stmt->expr) {
            b.emit2(b.code, kOpStore, var, emitExpr(stmt->expr.get()));
        }
        return true;
    }
    case kStmtExpr:
        emitExpr(stmt->expr.get());
        return true;
    case kStmtIf: {
        emitExpr(stmt->expr.get());
        if (stmt->thenStmt) {
            emitStmt(stmt->thenStmt.get(), returnType);
        }
        if (stmt->elseStmt) {
            emitStmt(stmt->elseStmt.get(), returnType);
        }
        return true;
    }
    case kStmtFor:
    case kStmtWhile:
    case kStmtDoWhile:
        if (stmt->thenStmt) {
            emitStmt(stmt->thenStmt.get(), returnType);
        }
        return true;
    default:
        return true;
    }
}

} /* namespace anonymous */

bool
emitFunctionSPIRV(
    const TranslationUnit &unit, const Function &fn, ShaderStage stage, std::vector<uint32_t> &words, std::string &error)
{
    Emitter e;
    e.unit = &unit;
    e.stage = stage;
    e.b.emit1(e.b.header, kOpCapability, kCapShader);
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
    std::vector<uint32_t> inTypes;
    for (size_t i = 0; i < fn.params.size(); i++) {
        uint32_t ty = e.b.typeOf(fn.params[i].type.kind == kTypeVector || fn.params[i].type.kind == kTypeFloat ||
                fn.params[i].type.kind == kTypeInt
            ? fn.params[i].type
            : Type::vectorType(kTypeFloat, 4));
        if (fn.params[i].type.kind != kTypeVector && fn.params[i].type.kind != kTypeFloat &&
            fn.params[i].type.kind != kTypeInt) {
            ty = e.b.typeVec(4);
        }
        uint32_t ptr = e.b.ptrType(ty, kStorageInput);
        uint32_t var = e.b.nextId();
        e.b.emit3(e.b.types, kOpVariable, ptr, var, kStorageInput);
        e.b.decorate(var, kDecorationLocation, static_cast<uint32_t>(semanticLocation(fn.params[i].semantic)), true);
        inVars.push_back(var);
        inTypes.push_back(ty);
        e.locals[fn.params[i].name] = var;
        e.localTypes[fn.params[i].name] = ty;
    }

    uint32_t outTy = retType;
    uint32_t outPtr = e.b.ptrType(outTy, kStorageOutput);
    uint32_t outVar = e.b.nextId();
    e.b.emit3(e.b.types, kOpVariable, outPtr, outVar, kStorageOutput);
    if (stage == kStageVertex && isPositionSemantic(fn.returnSemantic)) {
        e.b.decorate(outVar, kDecorationBuiltIn, kBuiltInPosition, true);
    }
    else {
        e.b.decorate(outVar, kDecorationLocation, 0, true);
    }

    int samplerIndex = 0;
    for (size_t i = 0; i < unit.variables.size(); i++) {
        if (!unit.variables[i].type.isSampler()) {
            continue;
        }
        uint32_t imageType = e.b.nextId();
        uint32_t sampledType = e.b.nextId();
        uint32_t ops[8] = { imageType, e.b.typeFloat(), kDim2D, 0, 0, 0, 1, 0 };
        e.b.emit(e.b.types, kOpTypeImage, ops, 8);
        e.b.emit2(e.b.types, kOpTypeSampledImage, sampledType, imageType);
        uint32_t ptr = e.b.ptrType(sampledType, kStorageUniformConstant);
        uint32_t var = e.b.nextId();
        e.b.emit3(e.b.types, kOpVariable, ptr, var, kStorageUniformConstant);
        int binding = samplerIndex;
        if (!unit.variables[i].registerName.empty() &&
            (unit.variables[i].registerName[0] == 's' || unit.variables[i].registerName[0] == 'S')) {
            binding = std::atoi(unit.variables[i].registerName.c_str() + 1);
        }
        e.b.decorate(var, kDecorationDescriptorSet, 0, true);
        e.b.decorate(var, kDecorationBinding, static_cast<uint32_t>(binding), true);
        e.samplers[unit.variables[i].name] = var;
        samplerIndex++;
        if (samplerIndex > 32) {
            break;
        }
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

    uint32_t retValue = e.pendingReturn ? e.pendingReturn : e.b.constVec4(0, 0, 0, 1);
    e.b.emit2(e.b.code, kOpStore, outVar, retValue);
    e.b.emit(e.b.code, kOpReturn, nullptr, 0);
    e.b.emit(e.b.code, kOpFunctionEnd, nullptr, 0);

    words.clear();
    words.push_back(0x07230203);
    words.push_back(0x00010000);
    words.push_back(0);
    words.push_back(e.b.bound);
    words.push_back(0);
    words.insert(words.end(), e.b.header.begin(), e.b.header.end());
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

} /* namespace fx9next */

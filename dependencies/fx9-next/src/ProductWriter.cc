/*
   Copyright (c) 2015-2023 hkrn All rights reserved

   This file is licensed under MIT license. for more details, see LICENSE.txt.
 */

#include "fx9next/ProductWriter.h"

#include "fx9next/RenderState.h"
#include "fx9next/SpirvEmitter.h"
#include "fx9next/Translator.h"

#include "effect.pb-c.h"

#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <exception>
#include <string>
#include <vector>

namespace fx9next {
namespace {

struct Arena {
    std::vector<void *> blocks;
    ~Arena()
    {
        for (size_t i = 0; i < blocks.size(); i++) {
            std::free(blocks[i]);
        }
    }
    template <typename T>
    T *
    alloc()
    {
        T *p = static_cast<T *>(std::calloc(1, sizeof(T)));
        blocks.push_back(p);
        return p;
    }
    template <typename T>
    T **
    allocArray(size_t n)
    {
        if (n == 0) {
            return nullptr;
        }
        T **p = static_cast<T **>(std::calloc(n, sizeof(T *)));
        blocks.push_back(p);
        return p;
    }
    char *
    copy(const std::string &s)
    {
        char *p = static_cast<char *>(std::malloc(s.size() + 1));
        std::memcpy(p, s.c_str(), s.size() + 1);
        blocks.push_back(p);
        return p;
    }
};

const Function *
findFn(const TranslationUnit &unit, const std::string &name)
{
    for (size_t i = 0; i < unit.functions.size(); i++) {
        if (unit.functions[i].name == name) {
            return &unit.functions[i];
        }
    }
    return nullptr;
}

const ShaderModuleIR *
findShader(const std::vector<ShaderModuleIR> &shaders, const std::string &entryPoint, ShaderStage stage)
{
    for (std::vector<ShaderModuleIR>::const_iterator it = shaders.begin(); it != shaders.end(); ++it) {
        if (it->entryPoint == entryPoint && it->stage == stage) {
            return &*it;
        }
    }
    return nullptr;
}

void
fillAnnotations(Arena &arena, const std::vector<Annotation> &src, Fx9__Effect__Annotation ***dst, size_t *n)
{
    *n = src.size();
    *dst = arena.allocArray<Fx9__Effect__Annotation>(*n);
    for (size_t i = 0; i < src.size(); i++) {
        Fx9__Effect__Annotation *a = (*dst)[i] = arena.alloc<Fx9__Effect__Annotation>();
        fx9__effect__annotation__init(a);
        a->name = arena.copy(src[i].name);
        if (src[i].kind == Annotation::kAnnString) {
            a->value_case = FX9__EFFECT__ANNOTATION__VALUE_SVAL_UTF8;
            a->sval_utf8 = arena.copy(src[i].sval);
        }
        else if (src[i].kind == Annotation::kAnnBool) {
            a->value_case = FX9__EFFECT__ANNOTATION__VALUE_BVAL;
            a->bval = src[i].bval ? 1 : 0;
        }
        else if (src[i].kind == Annotation::kAnnFloat) {
            a->value_case = FX9__EFFECT__ANNOTATION__VALUE_FVAL;
            a->fval = src[i].fval;
        }
        else {
            a->value_case = FX9__EFFECT__ANNOTATION__VALUE_IVAL;
            a->ival = src[i].ival;
        }
    }
}

void
fillShaderBody(Arena &arena, Fx9__Effect__Shader *shader, LanguageType language, const std::string &source,
    const std::vector<uint32_t> &spirv)
{
    if (language == kLanguageTypeHLSL) {
        shader->body_case = FX9__EFFECT__SHADER__BODY_HLSL;
        shader->hlsl = arena.copy(source);
    }
    else if (language == kLanguageTypeMSL) {
        shader->body_case = FX9__EFFECT__SHADER__BODY_MSL;
        shader->msl = arena.copy(source);
    }
    else if (language == kLanguageTypeSPIRV) {
        shader->body_case = FX9__EFFECT__SHADER__BODY_SPIRV;
        size_t bytes = spirv.size() * sizeof(uint32_t);
        uint8_t *data = static_cast<uint8_t *>(std::malloc(bytes ? bytes : 1));
        if (bytes) {
            std::memcpy(data, spirv.data(), bytes);
        }
        arena.blocks.push_back(data);
        shader->spirv.data = data;
        shader->spirv.len = bytes;
    }
    else {
        shader->body_case = FX9__EFFECT__SHADER__BODY_GLSL;
        shader->glsl = arena.copy(source);
    }
}

void
fillSamplers(Arena &arena, const TranslationUnit &unit, Fx9__Effect__Shader *shader)
{
    std::vector<const Variable *> samps;
    for (size_t i = 0; i < unit.variables.size(); i++) {
        if (unit.variables[i].type.isSampler()) {
            samps.push_back(&unit.variables[i]);
        }
    }
    shader->n_samplers = samps.size();
    shader->samplers = arena.allocArray<Fx9__Effect__Sampler>(shader->n_samplers);
    for (size_t i = 0; i < samps.size(); i++) {
        Fx9__Effect__Sampler *s = shader->samplers[i] = arena.alloc<Fx9__Effect__Sampler>();
        fx9__effect__sampler__init(s);
        s->type = FX9__EFFECT__SAMPLER__TYPE__SAMPLER_2D;
        if (samps[i]->type.samplerDim == kSamplerCube) {
            s->type = FX9__EFFECT__SAMPLER__TYPE__SAMPLER_CUBE;
        }
        else if (samps[i]->type.samplerDim == kSampler3D) {
            s->type = FX9__EFFECT__SAMPLER__TYPE__SAMPLER_VOLUME;
        }
        s->index = static_cast<uint32_t>(i);
        if (!samps[i]->registerName.empty() &&
            (samps[i]->registerName[0] == 's' || samps[i]->registerName[0] == 'S')) {
            s->index = static_cast<uint32_t>(std::atoi(samps[i]->registerName.c_str() + 1));
        }
        s->sampler_name = arena.copy(samps[i]->name);
        s->texture_name = arena.copy(samps[i]->textureName.empty() ? samps[i]->name : samps[i]->textureName);
    }
}

void
describeType(const Type &type, Fx9__Effect__Parameter *p)
{
    p->num_elements = type.arraySize > 0 ? static_cast<uint32_t>(type.arraySize) : 1;
    p->has_class_common = 1;
    p->has_type_common = 1;
    if (type.kind == kTypeSampler) {
        p->class_common = FX9__EFFECT__PARAMETER__CLASS_COMMON__PC_OBJECT;
        if (type.samplerDim == kSamplerCube) {
            p->type_common = FX9__EFFECT__PARAMETER__TYPE_COMMON__PT_SAMPLERCUBE;
        }
        else if (type.samplerDim == kSampler3D) {
            p->type_common = FX9__EFFECT__PARAMETER__TYPE_COMMON__PT_SAMPLER3D;
        }
        else if (type.samplerDim == kSampler1D) {
            p->type_common = FX9__EFFECT__PARAMETER__TYPE_COMMON__PT_SAMPLER1D;
        }
        else {
            p->type_common = FX9__EFFECT__PARAMETER__TYPE_COMMON__PT_SAMPLER2D;
        }
        p->num_rows = p->num_columns = 1;
        return;
    }
    if (type.kind == kTypeTexture) {
        p->class_common = FX9__EFFECT__PARAMETER__CLASS_COMMON__PC_OBJECT;
        if (type.samplerDim == kSamplerCube) {
            p->type_common = FX9__EFFECT__PARAMETER__TYPE_COMMON__PT_TEXTURECUBE;
        }
        else if (type.samplerDim == kSampler3D) {
            p->type_common = FX9__EFFECT__PARAMETER__TYPE_COMMON__PT_TEXTURE3D;
        }
        else {
            p->type_common = FX9__EFFECT__PARAMETER__TYPE_COMMON__PT_TEXTURE2D;
        }
        p->num_rows = p->num_columns = 1;
        return;
    }
    if (type.kind == kTypeString) {
        p->class_common = FX9__EFFECT__PARAMETER__CLASS_COMMON__PC_OBJECT;
        p->type_common = FX9__EFFECT__PARAMETER__TYPE_COMMON__PT_STRING;
        p->num_rows = p->num_columns = 1;
        return;
    }
    if (type.kind == kTypeBool || type.scalar == kTypeBool) {
        p->type_common = FX9__EFFECT__PARAMETER__TYPE_COMMON__PT_BOOL;
    }
    else if (type.kind == kTypeInt || type.kind == kTypeUInt || type.scalar == kTypeInt) {
        p->type_common = FX9__EFFECT__PARAMETER__TYPE_COMMON__PT_INT;
    }
    else {
        p->type_common = FX9__EFFECT__PARAMETER__TYPE_COMMON__PT_FLOAT;
    }
    if (type.kind == kTypeMatrix) {
        p->class_common = FX9__EFFECT__PARAMETER__CLASS_COMMON__PC_MATRIX_ROWS;
        p->num_rows = static_cast<uint32_t>(type.rows);
        p->num_columns = static_cast<uint32_t>(type.columns);
    }
    else if (type.kind == kTypeVector) {
        p->class_common = FX9__EFFECT__PARAMETER__CLASS_COMMON__PC_VECTOR;
        p->num_rows = 1;
        p->num_columns = static_cast<uint32_t>(type.rows);
    }
    else {
        p->class_common = FX9__EFFECT__PARAMETER__CLASS_COMMON__PC_SCALAR;
        p->num_rows = p->num_columns = 1;
    }
}

void
packLiteralValue(Arena &arena, const Variable &var, Fx9__Effect__Parameter *p)
{
    const uint32_t components = p->num_rows * p->num_columns * p->num_elements;
    const size_t bytes = sizeof(float) * (components > 0 ? components : 1);
    uint8_t *data = static_cast<uint8_t *>(std::calloc(1, bytes));
    arena.blocks.push_back(data);
    if (var.initializer) {
        const Expr *expr = var.initializer.get();
        if (expr->kind == kExprLiteralFloat || expr->kind == kExprLiteralInt || expr->kind == kExprLiteralBool) {
            float v = expr->kind == kExprLiteralBool ? (expr->boolValue ? 1.0f : 0.0f)
                                                     : (expr->kind == kExprLiteralInt ? static_cast<float>(expr->intValue)
                                                                                      : static_cast<float>(expr->floatValue));
            for (uint32_t i = 0; i < components; i++) {
                std::memcpy(data + sizeof(float) * i, &v, sizeof(float));
            }
        }
        else if (expr->kind == kExprConstruct) {
            for (size_t i = 0; i < expr->kids.size() && i < components; i++) {
                const Expr *kid = expr->kids[i].get();
                float v = 0;
                if (kid && (kid->kind == kExprLiteralFloat || kid->kind == kExprLiteralInt)) {
                    v = kid->kind == kExprLiteralInt ? static_cast<float>(kid->intValue)
                                                     : static_cast<float>(kid->floatValue);
                }
                std::memcpy(data + sizeof(float) * i, &v, sizeof(float));
            }
        }
    }
    p->value.data = data;
    p->value.len = bytes;
}

bool
isStructMarker(const Variable &var)
{
    return var.type.kind == kTypeStruct && var.semantic.empty() && var.type.name == var.name &&
        !var.type.members.empty();
}

void
fillParameters(Arena &arena, const TranslationUnit &unit, Fx9__Effect__Effect *message)
{
    std::vector<const Variable *> params;
    for (size_t i = 0; i < unit.variables.size(); i++) {
        if (!isStructMarker(unit.variables[i])) {
            params.push_back(&unit.variables[i]);
        }
    }
    message->n_parameters = params.size();
    message->parameters = arena.allocArray<Fx9__Effect__Parameter>(params.size());
    for (size_t i = 0; i < params.size(); i++) {
        const Variable &var = *params[i];
        Fx9__Effect__Parameter *p = message->parameters[i] = arena.alloc<Fx9__Effect__Parameter>();
        fx9__effect__parameter__init(p);
        p->name = arena.copy(var.name);
        p->semantic = arena.copy(var.semantic);
        p->flags = 0;
        describeType(var.type, p);
        fillAnnotations(arena, var.annotations, &p->annotations, &p->n_annotations);
        packLiteralValue(arena, var, p);
    }
}

std::string
upperCopy(const std::string &s)
{
    std::string o(s);
    for (size_t i = 0; i < o.size(); i++) {
        o[i] = static_cast<char>(std::toupper(static_cast<unsigned char>(o[i])));
    }
    return o;
}

bool
passStateEnabled(const Pass &pass, const char *name)
{
    const std::string key = upperCopy(name);
    for (size_t i = 0; i < pass.states.size(); i++) {
        if (upperCopy(pass.states[i].name) == key) {
            const std::string v = upperCopy(pass.states[i].value);
            return v == "TRUE" || v == "1" || v == "ENABLE";
        }
    }
    return false;
}

uint32_t
passStateU32(const Pass &pass, const char *name, uint32_t fallback)
{
    const std::string key = upperCopy(name);
    for (size_t i = 0; i < pass.states.size(); i++) {
        if (upperCopy(pass.states[i].name) == key) {
            uint32_t value = 0;
            if (lookupRenderStateValue(pass.states[i].name, pass.states[i].value, value)) {
                return value;
            }
        }
    }
    return fallback;
}

std::string
alphaTestCondition(const Pass &pass)
{
    const uint32_t ref = passStateU32(pass, "ALPHAREF", 0);
    const float reference = static_cast<float>(ref) / 255.0f;
    const uint32_t func = passStateU32(pass, "ALPHAFUNC", 8);
    char buf[64];
    switch (func) {
    case 1:
        return "false";
    case 2:
        std::snprintf(buf, sizeof(buf), "(nanoem_output_color.a < %.8ff)", reference);
        return buf;
    case 3:
        std::snprintf(buf, sizeof(buf), "(nanoem_output_color.a == %.8ff)", reference);
        return buf;
    case 4:
        std::snprintf(buf, sizeof(buf), "(nanoem_output_color.a <= %.8ff)", reference);
        return buf;
    case 5:
        std::snprintf(buf, sizeof(buf), "(nanoem_output_color.a > %.8ff)", reference);
        return buf;
    case 6:
        std::snprintf(buf, sizeof(buf), "(nanoem_output_color.a != %.8ff)", reference);
        return buf;
    case 7:
        std::snprintf(buf, sizeof(buf), "(nanoem_output_color.a >= %.8ff)", reference);
        return buf;
    default:
        return "true";
    }
}

bool
replaceOutputAssignment(std::string &source, const char *needle, const std::string &lhsType, bool alphaTest,
    bool srgbWrite, const std::string &condition)
{
    const size_t at = source.find(needle);
    if (at == std::string::npos) {
        return false;
    }
    const size_t expr = at + std::strlen(needle);
    const size_t semi = source.find(';', expr);
    if (semi == std::string::npos) {
        return false;
    }
    const std::string original = source.substr(expr, semi - expr);
    std::string replacement;
    replacement += lhsType;
    replacement += " nanoem_output_color = ";
    replacement += original;
    replacement += ";\n";
    if (alphaTest) {
        replacement += "    if (!(";
        replacement += condition;
        replacement += ")) { discard; }\n";
    }
    if (srgbWrite) {
        replacement += "    /* 0.0031308 */\n";
        replacement += "    ";
        replacement += lhsType == "float4" ? "float3" : "vec3";
        replacement += " nanoem_output_linear = ";
        replacement += lhsType == "float4" ? "saturate(nanoem_output_color.rgb)" : "clamp(nanoem_output_color.rgb, 0.0, 1.0)";
        replacement += ";\n    nanoem_output_color.rgb = ";
        replacement += lhsType == "float4" ? "float3" : "vec3";
        replacement += "(\n";
        replacement += "        nanoem_output_linear.x <= 0.0031308 ? nanoem_output_linear.x * 12.92 : "
                       "1.055 * pow(nanoem_output_linear.x, 1.0 / 2.4) - 0.055,\n";
        replacement += "        nanoem_output_linear.y <= 0.0031308 ? nanoem_output_linear.y * 12.92 : "
                       "1.055 * pow(nanoem_output_linear.y, 1.0 / 2.4) - 0.055,\n";
        replacement += "        nanoem_output_linear.z <= 0.0031308 ? nanoem_output_linear.z * 12.92 : "
                       "1.055 * pow(nanoem_output_linear.z, 1.0 / 2.4) - 0.055);\n";
    }
    replacement += "    ";
    replacement += needle;
    replacement += "nanoem_output_color";
    source.replace(at, semi - at, replacement);
    return true;
}

std::string
findFragOutputName(const std::string &src)
{
    static const char *kKeys[] = { "layout(location = 0) out vec4 ", "layout(location=0) out vec4 ", "out vec4 ",
        "layout(location = 0) out float4 ", nullptr };
    for (const char **k = kKeys; *k; ++k) {
        const size_t at = src.find(*k);
        if (at == std::string::npos) {
            continue;
        }
        size_t i = at + std::strlen(*k);
        while (i < src.size() && std::isspace(static_cast<unsigned char>(src[i]))) {
            i++;
        }
        size_t start = i;
        while (i < src.size() && (std::isalnum(static_cast<unsigned char>(src[i])) || src[i] == '_')) {
            i++;
        }
        if (i > start) {
            return src.substr(start, i - start);
        }
    }
    return std::string();
}

void
bakePixelOutput(LanguageType language, const Pass &pass, std::string &psSrc)
{
    const bool alphaTest = passStateEnabled(pass, "ALPHATESTENABLE");
    const bool srgbWrite = passStateEnabled(pass, "SRGBWRITEENABLE");
    if (!alphaTest && !srgbWrite) {
        return;
    }
    const std::string condition = alphaTestCondition(pass);
    const bool hlsl = language == kLanguageTypeHLSL || language == kLanguageTypeMSL;
    const char *type = hlsl ? "float4" : "vec4";
    static const char *kNeedles[] = { "_RESERVED_IDENTIFIER_FIXUP_gl_FragData[0] = ", "gl_FragData[0] = ",
        "gl_FragColor = ", "FragColor = ", "_entryPointOutput = ", nullptr };
    for (const char **n = kNeedles; *n; ++n) {
        if (replaceOutputAssignment(psSrc, *n, type, alphaTest, srgbWrite, condition)) {
            return;
        }
    }
    const std::string outName = findFragOutputName(psSrc);
    if (!outName.empty()) {
        const std::string needle = outName + " = ";
        replaceOutputAssignment(psSrc, needle.c_str(), type, alphaTest, srgbWrite, condition);
    }
}

Fx9__Effect__Attribute__Usage
usageFromSemantic(const std::string &semantic)
{
    const std::string u = upperCopy(semantic);
    if (u.compare(0, 8, "POSITION") == 0 || u == "SV_POSITION") {
        return FX9__EFFECT__ATTRIBUTE__USAGE__AU_POSITION;
    }
    if (u.compare(0, 6, "NORMAL") == 0) {
        return FX9__EFFECT__ATTRIBUTE__USAGE__AU_NORMAL;
    }
    if (u.compare(0, 8, "TEXCOORD") == 0) {
        return FX9__EFFECT__ATTRIBUTE__USAGE__AU_TEXCOORD;
    }
    if (u.compare(0, 5, "COLOR") == 0) {
        return FX9__EFFECT__ATTRIBUTE__USAGE__AU_COLOR;
    }
    if (u.compare(0, 7, "TANGENT") == 0) {
        return FX9__EFFECT__ATTRIBUTE__USAGE__AU_TANGENT;
    }
    if (u.compare(0, 8, "BINORMAL") == 0) {
        return FX9__EFFECT__ATTRIBUTE__USAGE__AU_BINORMAL;
    }
    return FX9__EFFECT__ATTRIBUTE__USAGE__AU_TEXCOORD;
}

uint32_t
semanticIndex(const std::string &semantic)
{
    const std::string u = upperCopy(semantic);
    size_t i = u.size();
    while (i > 0 && std::isdigit(static_cast<unsigned char>(u[i - 1]))) {
        i--;
    }
    if (i == u.size()) {
        return 0;
    }
    return static_cast<uint32_t>(std::atoi(u.c_str() + i));
}

void
fillShaderInterface(Arena &arena, const TranslationUnit &unit, const Function &fn, Fx9__Effect__Shader *shader)
{
    shader->n_inputs = fn.params.size();
    shader->inputs = arena.allocArray<Fx9__Effect__Attribute>(fn.params.size());
    shader->n_semantics = fn.params.size();
    shader->semantics = arena.allocArray<Fx9__Effect__Semantic>(fn.params.size());
    for (size_t i = 0; i < fn.params.size(); i++) {
        Fx9__Effect__Attribute *attr = shader->inputs[i] = arena.alloc<Fx9__Effect__Attribute>();
        fx9__effect__attribute__init(attr);
        attr->name = arena.copy(fn.params[i].name);
        attr->usage = usageFromSemantic(fn.params[i].semantic);
        attr->index = semanticIndex(fn.params[i].semantic);
        Fx9__Effect__Semantic *sem = shader->semantics[i] = arena.alloc<Fx9__Effect__Semantic>();
        fx9__effect__semantic__init(sem);
        sem->index = attr->index;
        sem->input_name = arena.copy(fn.params[i].semantic.empty() ? fn.params[i].name : fn.params[i].semantic);
        sem->parameter_name = arena.copy(fn.params[i].name);
        if (attr->usage == FX9__EFFECT__ATTRIBUTE__USAGE__AU_POSITION) {
            sem->output_name = arena.copy("SV_POSITION");
        }
        else if (attr->usage == FX9__EFFECT__ATTRIBUTE__USAGE__AU_NORMAL) {
            sem->output_name = arena.copy("NORMAL");
        }
        else if (attr->usage == FX9__EFFECT__ATTRIBUTE__USAGE__AU_TEXCOORD) {
            sem->output_name = arena.copy("TEXCOORD");
        }
        else if (attr->usage == FX9__EFFECT__ATTRIBUTE__USAGE__AU_COLOR) {
            sem->output_name = arena.copy("COLOR");
        }
        else {
            sem->output_name = arena.copy(sem->input_name);
        }
    }
    std::vector<const Variable *> uniforms;
    for (size_t i = 0; i < unit.variables.size(); i++) {
        const Variable &var = unit.variables[i];
        if (isStructMarker(var) || var.type.isSampler() || var.type.kind == kTypeTexture ||
            var.type.kind == kTypeString) {
            continue;
        }
        uniforms.push_back(&var);
    }
    shader->n_uniforms = uniforms.size();
    shader->uniforms = arena.allocArray<Fx9__Effect__Uniform>(uniforms.size());
    shader->n_symbols = uniforms.size();
    shader->symbols = arena.allocArray<Fx9__Effect__Symbol>(uniforms.size());
    uint32_t reg = 0;
    for (size_t i = 0; i < uniforms.size(); i++) {
        Fx9__Effect__Uniform *u = shader->uniforms[i] = arena.alloc<Fx9__Effect__Uniform>();
        fx9__effect__uniform__init(u);
        u->type = FX9__EFFECT__UNIFORM__TYPE__UT_FLOAT;
        u->index = reg;
        u->num_elements = uniforms[i]->type.kind == kTypeMatrix ? static_cast<uint32_t>(uniforms[i]->type.rows) : 1;
        u->constant_index = reg;
        u->name = arena.copy(uniforms[i]->name);
        Fx9__Effect__Symbol *sym = shader->symbols[i] = arena.alloc<Fx9__Effect__Symbol>();
        fx9__effect__symbol__init(sym);
        sym->name = arena.copy(uniforms[i]->name);
        sym->register_set = FX9__EFFECT__SYMBOL__REGISTER_SET__RS_FLOAT4;
        sym->register_index = reg;
        sym->register_count = u->num_elements;
        reg += u->num_elements;
    }
}

void
fillIncludes(Arena &arena, const TranslationUnit &unit, Fx9__Effect__Effect *message)
{
    message->n_includes = unit.includes.size();
    message->includes = arena.allocArray<Fx9__Effect__Include>(unit.includes.size());
    for (size_t i = 0; i < unit.includes.size(); i++) {
        Fx9__Effect__Include *inc = message->includes[i] = arena.alloc<Fx9__Effect__Include>();
        fx9__effect__include__init(inc);
        inc->location = arena.copy(unit.includes[i]);
    }
}

} /* namespace anonymous */

bool
writeEffectProduct(const TranslationUnit &unit, const std::vector<ShaderModuleIR> &shaders, LanguageType language,
    const std::string &metalEntry, const std::string &metalUbo, int version, bool /*validate*/, EffectProduct &product)
{
    Arena arena;
    Fx9__Effect__Effect message = FX9__EFFECT__EFFECT__INIT;
    Fx9__Effect__Metadata *meta = arena.alloc<Fx9__Effect__Metadata>();
    fx9__effect__metadata__init(meta);
    meta->name = arena.copy("generator");
    meta->value = arena.copy("fx9-next");
    message.n_metadata = 1;
    message.metadata = arena.allocArray<Fx9__Effect__Metadata>(1);
    message.metadata[0] = meta;

    fillParameters(arena, unit, &message);
    fillIncludes(arena, unit, &message);

    message.n_techniques = unit.techniques.size();
    message.techniques = arena.allocArray<Fx9__Effect__Technique>(message.n_techniques);

    TranslateOptions options;
    options.language = language;
    options.version = version;
    options.metalEntry = metalEntry.empty() ? "fx9_metal_main" : metalEntry;
    options.metalUbo = metalUbo.empty() ? "nanoem_uniforms" : metalUbo;

    for (size_t ti = 0; ti < unit.techniques.size(); ti++) {
        const Technique &technique = unit.techniques[ti];
        Fx9__Effect__Technique *tech = message.techniques[ti] = arena.alloc<Fx9__Effect__Technique>();
        fx9__effect__technique__init(tech);
        tech->name = arena.copy(technique.name);
        fillAnnotations(arena, technique.annotations, &tech->annotations, &tech->n_annotations);
        tech->n_passes = technique.passes.size();
        tech->passes = arena.allocArray<Fx9__Effect__Pass>(tech->n_passes);
        for (size_t pi = 0; pi < technique.passes.size(); pi++) {
            const Pass &pass = technique.passes[pi];
            Fx9__Effect__Pass *passMsg = tech->passes[pi] = arena.alloc<Fx9__Effect__Pass>();
            fx9__effect__pass__init(passMsg);
            passMsg->name = arena.copy(pass.name);
            fillAnnotations(arena, pass.annotations, &passMsg->annotations, &passMsg->n_annotations);
            product.numPasses++;

            std::vector<Fx9__Effect__RenderState *> states;
            for (size_t si = 0; si < pass.states.size(); si++) {
                uint32_t key = 0;
                uint32_t value = 0;
                if (!lookupRenderStateKey(pass.states[si].name, key)) {
                    continue;
                }
                if (pass.states[si].value.empty()) {
                    continue;
                }
                if (!lookupRenderStateValue(pass.states[si].name, pass.states[si].value, value)) {
                    continue;
                }
                Fx9__Effect__RenderState *rs = arena.alloc<Fx9__Effect__RenderState>();
                fx9__effect__render_state__init(rs);
                rs->key = key;
                rs->value = value;
                states.push_back(rs);
            }
            passMsg->n_render_states = states.size();
            passMsg->render_states = arena.allocArray<Fx9__Effect__RenderState>(states.size());
            for (size_t si = 0; si < states.size(); si++) {
                passMsg->render_states[si] = states[si];
            }

            passMsg->vertex_shader = arena.alloc<Fx9__Effect__Shader>();
            fx9__effect__shader__init(passMsg->vertex_shader);
            passMsg->vertex_shader->type = FX9__EFFECT__SHADER__TYPE__ST_VERTEX;
            passMsg->vertex_shader->uniform_block_name = arena.copy("vs_uniforms_vec4");
            passMsg->pixel_shader = arena.alloc<Fx9__Effect__Shader>();
            fx9__effect__shader__init(passMsg->pixel_shader);
            passMsg->pixel_shader->type = FX9__EFFECT__SHADER__TYPE__ST_PIXEL;
            passMsg->pixel_shader->uniform_block_name = arena.copy("ps_uniforms_vec4");
            fillSamplers(arena, unit, passMsg->pixel_shader);

            if (pass.vsEntry.empty() && pass.psEntry.empty()) {
                product.numCompiledPasses++;
                product.numValidatedPasses++;
                continue;
            }
            const Function *vs = findFn(unit, pass.vsEntry);
            const Function *ps = findFn(unit, pass.psEntry);
            const ShaderModuleIR *vsIR = findShader(shaders, pass.vsEntry, kShaderStageVertex);
            const ShaderModuleIR *psIR = findShader(shaders, pass.psEntry, kShaderStagePixel);
            if (!vs || !ps || !vsIR || !psIR) {
                std::string names;
                for (size_t fi = 0; fi < unit.functions.size(); fi++) {
                    if (!names.empty()) {
                        names += ",";
                    }
                    names += unit.functions[fi].name;
                }
                product.sink.info = "missing shader entry vs='" + pass.vsEntry + "' ps='" + pass.psEntry +
                    "' fns=" + names;
                continue;
            }
            std::vector<uint32_t> vsWords, psWords;
            std::string err;
            try {
                if (!emitShaderSPIRV(unit, *vsIR, vsWords, err)) {
                    product.sink.builder = err.empty() ? "vertex emit failed" : err;
                    continue;
                }
                if (!emitShaderSPIRV(unit, *psIR, psWords, err)) {
                    product.sink.builder = err.empty() ? "fragment emit failed" : err;
                    continue;
                }
                std::string vsSrc, psSrc;
                std::vector<uint32_t> vsOut, psOut;
                if (!translateSPIRV(vsWords, psWords, options, vsSrc, psSrc, vsOut, psOut, err)) {
                    product.sink.translator.insert(err.empty() ? "translate failed" : err);
                    continue;
                }
                if (language != kLanguageTypeSPIRV && (vsSrc.empty() || psSrc.empty())) {
                    product.sink.translator.insert("empty translated source vs=" + std::to_string(vsSrc.size()) +
                        " ps=" + std::to_string(psSrc.size()) + " vspirv=" + std::to_string(vsWords.size()) +
                        " pspirv=" + std::to_string(psWords.size()));
                    continue;
                }
                bakePixelOutput(language, pass, psSrc);
                fillShaderInterface(arena, unit, *vs, passMsg->vertex_shader);
                fillShaderInterface(arena, unit, *ps, passMsg->pixel_shader);
                fillShaderBody(arena, passMsg->vertex_shader, language, vsSrc, vsOut);
                fillShaderBody(arena, passMsg->pixel_shader, language, psSrc, psOut);
                product.numCompiledPasses++;
                product.numValidatedPasses++;
            }
            catch (const std::exception &ex) {
                product.sink.info = std::string("exception: ") + ex.what();
            }
            catch (...) {
                product.sink.info = "unknown exception during pass compile";
            }
        }
    }

    size_t packed = fx9__effect__effect__get_packed_size(&message);
    product.message.resize(packed);
    if (packed > 0) {
        fx9__effect__effect__pack(&message, product.message.data());
    }
    return product.numPasses == 0 ||
        (product.numCompiledPasses == product.numPasses && product.numValidatedPasses == product.numPasses);
}

} /* namespace fx9next */

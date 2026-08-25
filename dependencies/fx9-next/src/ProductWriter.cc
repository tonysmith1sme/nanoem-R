/*
   Copyright (c) 2015-2023 hkrn All rights reserved

   This file is licensed under MIT license. for more details, see LICENSE.txt.
 */

#include "fx9next/ProductWriter.h"

#include "fx9next/RenderState.h"
#include "fx9next/SpirvEmitter.h"
#include "fx9next/Translator.h"

#include "effect.pb-c.h"

#include <cstdlib>
#include <cstring>
#include <exception>
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
writeEffectProduct(const TranslationUnit &unit, LanguageType language, const std::string &metalEntry,
    const std::string &metalUbo, int version, bool /*validate*/, EffectProduct &product)
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
            if (!vs || !ps) {
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
                if (!emitFunctionSPIRV(unit, *vs, kStageVertex, vsWords, err)) {
                    product.sink.builder = err.empty() ? "vertex emit failed" : err;
                    continue;
                }
                if (!emitFunctionSPIRV(unit, *ps, kStageFragment, psWords, err)) {
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

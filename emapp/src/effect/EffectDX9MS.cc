/*
  Copyright (c) 2015-2023 hkrn All rights reserved

  This file is part of emapp component and it's licensed under Mozilla Public License. see LICENSE.md for more details.
*/

#include "emapp/StringUtils.h"
#include "emapp/effect/Common.h"
#include "emapp/private/CommonInclude.h"

#include "./EffectCommon.inl"

namespace nanoem {
namespace effect {
namespace dx9ms {

struct HLSLField {
    HLSLField(const String &name, const String &semantic, int index)
        : m_name(name)
        , m_semantic(semantic)
        , m_index(index)
    {
    }
    String m_name;
    String m_semantic;
    int m_index;
};

static void
appendIndexedSemantic(String &value, const char *base, int index)
{
    char buffer[64];
    if (index > 0 || StringUtils::equals(base, "SV_Target")) {
        StringUtils::format(buffer, sizeof(buffer), "%s%d", base, index);
    }
    else {
        StringUtils::copyString(buffer, base, sizeof(buffer));
    }
    value.append(buffer);
}

static void
appendField(String &value, const char *type, const HLSLField &field)
{
    value.append("    ");
    value.append(type);
    value.append(" ");
    value.append(field.m_name.c_str());
    value.append(" : ");
    appendIndexedSemantic(value, field.m_semantic.c_str(), field.m_index);
    value.append(";\n");
}

static void
collectVertexInputFields(const Fx9__Effect__Dx9ms__Shader *shaderPtr, tinystl::vector<HLSLField, TinySTLAllocator> &fields)
{
    const size_t numAttributes = shaderPtr->n_attributes;
    for (size_t i = 0; i < numAttributes; i++) {
        const Fx9__Effect__Dx9ms__Attribute *attributePtr = shaderPtr->attributes[i];
        switch (attributePtr->usage) {
        case FX9__EFFECT__DX9MS__ATTRIBUTE_USAGE__AU_POSITION:
            fields.push_back(HLSLField("a_position", "POSITION", 0));
            break;
        case FX9__EFFECT__DX9MS__ATTRIBUTE_USAGE__AU_NORMAL:
            fields.push_back(HLSLField("a_normal", "NORMAL", 0));
            break;
        case FX9__EFFECT__DX9MS__ATTRIBUTE_USAGE__AU_TEXCOORD: {
            char buffer[32];
            StringUtils::format(buffer, sizeof(buffer), "a_texcoord%d", attributePtr->index);
            fields.push_back(HLSLField(buffer, "TEXCOORD", attributePtr->index));
            break;
        }
        case FX9__EFFECT__DX9MS__ATTRIBUTE_USAGE__AU_COLOR: {
            char buffer[32];
            StringUtils::format(buffer, sizeof(buffer), "a_color%d", attributePtr->index);
            fields.push_back(HLSLField(buffer, "COLOR", attributePtr->index));
            break;
        }
        case FX9__EFFECT__DX9MS__ATTRIBUTE_USAGE__AU_BLENDWEIGHT:
            fields.push_back(HLSLField("a_weight", "BLENDWEIGHT", 0));
            break;
        default:
            break;
        }
    }
}

static void
collectVertexOutputFields(const Fx9__Effect__Dx9ms__Shader *shaderPtr, tinystl::vector<HLSLField, TinySTLAllocator> &fields)
{
    fields.push_back(HLSLField("position", "SV_Position", 0));
    const size_t numOutputs = shaderPtr->n_outputs;
    for (size_t i = 0; i < numOutputs; i++) {
        const Fx9__Effect__Dx9ms__Attribute *outputPtr = shaderPtr->outputs[i];
        switch (outputPtr->usage) {
        case FX9__EFFECT__DX9MS__ATTRIBUTE_USAGE__AU_TEXCOORD:
            fields.push_back(HLSLField(outputPtr->name, "TEXCOORD", outputPtr->index));
            break;
        case FX9__EFFECT__DX9MS__ATTRIBUTE_USAGE__AU_COLOR:
            fields.push_back(HLSLField(outputPtr->name, "COLOR", outputPtr->index));
            break;
        default:
            break;
        }
    }
}

static void
collectPixelInputFields(const StringMap &shaderOutputVariables, tinystl::vector<HLSLField, TinySTLAllocator> &fields)
{
    StringSet processed;
    for (StringMap::const_iterator it = shaderOutputVariables.begin(), end = shaderOutputVariables.end(); it != end; ++it) {
        const String &mappedName = it->second;
        if (processed.find(mappedName) != processed.end()) {
            continue;
        }
        const String &source = it->first;
        if (StringUtils::equals(source.c_str(), "gl_Color")) {
            fields.push_back(HLSLField(mappedName, "COLOR", 0));
        }
        else if (StringUtils::equals(source.c_str(), "gl_SecondaryColor")) {
            fields.push_back(HLSLField(mappedName, "COLOR", 1));
        }
        else if (StringUtils::hasPrefix(source.c_str(), "gl_TexCoord[")) {
            const char *ptr = source.c_str() + 12;
            int index = StringUtils::parseInteger(ptr, nullptr);
            fields.push_back(HLSLField(mappedName, "TEXCOORD", index));
        }
        else if (StringUtils::equals(source.c_str(), "gl_TexCoord")) {
            fields.push_back(HLSLField(mappedName, "TEXCOORD", 0));
        }
        processed.insert(mappedName);
    }
}

static void
appendSamplerDeclarations(const Fx9__Effect__Dx9ms__Shader *shaderPtr, String &value)
{
    for (size_t i = 0, numSamplers = shaderPtr->n_samplers; i < numSamplers; i++) {
        const Fx9__Effect__Dx9ms__Sampler *samplerPtr = shaderPtr->samplers[i];
        const nanoem_u32_t index = samplerPtr->index;
        const char *textureType = "Texture2D";
        switch (samplerPtr->type) {
        case FX9__EFFECT__DX9MS__SAMPLER_TYPE__SAMPLER_CUBE:
            textureType = "TextureCube";
            break;
        case FX9__EFFECT__DX9MS__SAMPLER_TYPE__SAMPLER_VOLUME:
            textureType = "Texture3D";
            break;
        case FX9__EFFECT__DX9MS__SAMPLER_TYPE__SAMPLER_2D:
        default:
            break;
        }
        char buffer[128];
        StringUtils::format(buffer, sizeof(buffer), "%s %s_texture : register(t%d);\n", textureType,
            samplerPtr->name, index);
        value.append(buffer);
        StringUtils::format(buffer, sizeof(buffer), "SamplerState %s : register(s%d);\n", samplerPtr->name, index);
        value.append(buffer);
    }
}

static void
appendUniformBufferDeclaration(const Fx9__Effect__Dx9ms__Shader *shaderPtr, const char *name, String &value)
{
    nanoem_u32_t maxUniformIndices = 0;
    for (size_t i = 0, numUniforms = shaderPtr->n_uniforms; i < numUniforms; i++) {
        const Fx9__Effect__Dx9ms__Uniform *uniformPtr = shaderPtr->uniforms[i];
        maxUniformIndices = glm::max(maxUniformIndices, uniformPtr->index);
    }
    char buffer[128];
    StringUtils::format(buffer, sizeof(buffer), "cbuffer %s_t : register(b0) { float4 %s[%d]; };\n", name, name,
        maxUniformIndices + 1);
    value.append(buffer);
}

static void
replaceAllString(String &value, const char *from, const char *to)
{
    if (!from || !to) {
        return;
    }
    const nanoem_rsize_t fromLength = StringUtils::length(from), toLength = StringUtils::length(to);
    if (fromLength == 0) {
        return;
    }
    String rebuilt;
    const char *begin = value.c_str();
    const char *search = begin;
    while (const char *target = bx::strFind(search, from)) {
        rebuilt.append(search, target);
        rebuilt.append(to, toLength);
        search = target + fromLength;
    }
    rebuilt.append(search, begin + value.size());
    value = rebuilt;
}

static void
appendCommonHLSLDefines(String &value)
{
    value.append("#define highp\n");
    value.append("#define mediump\n");
    value.append("#define middlep\n");
    value.append("#define lowp\n");
    value.append("#define half float\n");
    value.append("#define half2 float2\n");
    value.append("#define half3 float3\n");
    value.append("#define half4 float4\n");
    value.append("#define fixed float\n");
    value.append("#define fixed2 float2\n");
    value.append("#define fixed3 float3\n");
    value.append("#define fixed4 float4\n");
    value.append("#define vec2 float2\n");
    value.append("#define vec3 float3\n");
    value.append("#define vec4 float4\n");
    value.append("#define mat2 float2x2\n");
    value.append("#define ivec2 int2\n");
    value.append("#define ivec3 int3\n");
    value.append("#define ivec4 int4\n");
    value.append("#define bvec2 bool2\n");
    value.append("#define bvec3 bool3\n");
    value.append("#define bvec4 bool4\n");
    value.append("#define mat3 float3x3\n");
    value.append("#define mat4 float4x4\n");
    value.append("#define mix lerp\n");
    value.append("#define fract frac\n");
    value.append("#define mod fmod\n");
    value.append("#define texture2D(s, v) s##_texture.Sample(s, (v).xy)\n");
    value.append("#define texture2DLod(s, v) s##_texture.SampleLevel(s, (v).xy, (v).w)\n");
    value.append("#define tex2D(s, v) s##_texture.Sample(s, (v).xy)\n");
    value.append("#define tex2Dbias(s, v) s##_texture.SampleBias(s, (v).xy, (v).w)\n");
    value.append("#define tex2Dlod(s, v) s##_texture.SampleLevel(s, (v).xy, (v).w)\n");
    value.append("#define tex2Dproj(s, v) s##_texture.Sample(s, (v).xy / (v).w)\n");
    value.append("#define texCUBE(s, v) s##_texture.Sample(s, (v).xyz)\n");
    value.append("#define texCUBElod(s, v) s##_texture.SampleLevel(s, (v).xyz, (v).w)\n");
    value.append("#define tex3D(s, v) s##_texture.Sample(s, (v).xyz)\n");
    value.append("#define tex3Dlod(s, v) s##_texture.SampleLevel(s, (v).xyz, (v).w)\n");
    value.append("#define tex1D(s, v) s##_texture.Sample(s, float2((v), 0.0))\n");
    value.append("#define tex1Dlod(s, v) s##_texture.SampleLevel(s, float2((v).x, 0.0), (v).w)\n");
}

static void
replaceFragmentDataReferences(String &value)
{
    replaceAllString(value, "gl_FragData[0]", "output.o_color0");
    replaceAllString(value, "gl_FragData[1]", "output.o_color1");
    replaceAllString(value, "gl_FragData[2]", "output.o_color2");
    replaceAllString(value, "gl_FragData[3]", "output.o_color3");
}

static void
appendProcessedHLSLBody(const Fx9__Effect__Dx9ms__Shader *shaderPtr, bool isVertexShader, String &value)
{
    const size_t numUniforms = shaderPtr->n_uniforms;
    static const char kUniformPrefix[] = "uniform vec4 ";
    static const char kVersionPrefix[] = "#version ";
    static const char kAttributePrefix[] = "attribute ";
    static const char kVaryingPrefix[] = "varying ";
    static const char kOutPrefix[] = "out vec4 ";
    static const char kDefineVSCPrefix[] = "#define vs_c";
    static const char kDefinePSCPrefix[] = "#define ps_c";
    static const char kDefinePSVPrefix[] = "#define ps_v";
    static const char kDefineVSOPrefix[] = "#define vs_o";
    BX_UNUSED_1(numUniforms);
    bx::LineReader reader(shaderPtr->code);
    int braceDepth = 0;
    bool insertedReturn = false;
    while (!reader.isDone()) {
        const bx::StringView line = reader.next();
        const char *linePtr = line.getPtr();
        String current(linePtr, line.getLength());
        if (current.empty() || *current.c_str() == '\r') {
            continue;
        }
        if (StringUtils::hasPrefix(linePtr, kVersionPrefix) || StringUtils::hasPrefix(linePtr, kAttributePrefix) ||
            StringUtils::hasPrefix(linePtr, kVaryingPrefix) || StringUtils::hasPrefix(linePtr, kOutPrefix) ||
            StringUtils::hasPrefix(linePtr, kUniformPrefix)) {
            continue;
        }
        if (StringUtils::hasPrefix(linePtr, kDefineVSOPrefix)) {
            continue;
        }
        if (StringUtils::hasPrefix(linePtr, kDefineVSCPrefix) || StringUtils::hasPrefix(linePtr, kDefinePSCPrefix)) {
            const bool isVS = StringUtils::hasPrefix(linePtr, kDefineVSCPrefix);
            const bx::StringView basePtr(line, sizeof(kDefineVSCPrefix) - 1);
            char *mutablePtr = nullptr;
            int uniformIndex = StringUtils::parseInteger(basePtr.getPtr(), &mutablePtr);
            const bx::StringView fromPtr = bx::strLTrimSpace(mutablePtr), toPtr = bx::strWord(fromPtr);
            char variableName[64];
            StringUtils::copyString(variableName, fromPtr.getPtr(),
                glm::min(Inline::saturateInt32(sizeof(variableName)), toPtr.getLength()));
            char buffer[128];
            StringUtils::format(buffer, sizeof(buffer), "#define %s%d %s[%d]\n", isVS ? "vs_c" : "ps_c",
                uniformIndex, isVS ? "vs_uniforms_vec" : "ps_uniforms_vec", uniformIndex);
            value.append(buffer);
            continue;
        }
        if (!isVertexShader && StringUtils::hasPrefix(linePtr, kDefinePSVPrefix)) {
            char variableName[64], mappedName[64];
            const bx::StringView basePtr(line, sizeof(kDefinePSCPrefix) - 1);
            char *mutablePtr = nullptr;
            int variableIndex = StringUtils::parseInteger(basePtr.getPtr(), &mutablePtr);
            StringUtils::format(variableName, sizeof(variableName), "ps_v%d", variableIndex);
            const bx::StringView fromPtr = bx::strLTrimSpace(mutablePtr);
            const char *strPtr = fromPtr.getPtr();
            while (strPtr != line.getTerm() &&
                (bx::isAlphaNum(*strPtr) || *strPtr == '_' || *strPtr == '[' || *strPtr == ']')) {
                strPtr++;
            }
            StringUtils::copyString(mappedName, fromPtr.getPtr(), size_t(strPtr - fromPtr.getPtr() + 1));
            String fieldName;
            if (StringUtils::equals(mappedName, "gl_Color")) {
                fieldName = "input.v_color0";
            }
            else if (StringUtils::equals(mappedName, "gl_SecondaryColor")) {
                fieldName = "input.v_color1";
            }
            else if (StringUtils::hasPrefix(mappedName, "gl_TexCoord[")) {
                int index = StringUtils::parseInteger(mappedName + 12, nullptr);
                char buffer[64];
                StringUtils::format(buffer, sizeof(buffer), "input.v_texcoord%d", index);
                fieldName = buffer;
            }
            else if (StringUtils::equals(mappedName, "gl_TexCoord")) {
                fieldName = "input.v_texcoord0";
            }
            if (!fieldName.empty()) {
                value.append("#define ");
                value.append(variableName);
                value.append(" ");
                value.append(fieldName.c_str());
                value.append("\n");
            }
            continue;
        }
        replaceAllString(current, "void main()", isVertexShader ? "VS_OUTPUT main(VS_INPUT input)" : "PS_OUTPUT main(PS_INPUT input)");
        replaceAllString(current, "void main(void)", isVertexShader ? "VS_OUTPUT main(VS_INPUT input)" : "PS_OUTPUT main(PS_INPUT input)");
        replaceFragmentDataReferences(current);
        replaceAllString(current, "gl_FragColor", "output.o_color0");
        replaceAllString(current, "vec2", "float2");
        replaceAllString(current, "vec3", "float3");
        replaceAllString(current, "vec4", "float4");
        replaceAllString(current, "half2", "float2");
        replaceAllString(current, "half3", "float3");
        replaceAllString(current, "half4", "float4");
        replaceAllString(current, "half", "float");
        replaceAllString(current, "fixed2", "float2");
        replaceAllString(current, "fixed3", "float3");
        replaceAllString(current, "fixed4", "float4");
        replaceAllString(current, "fixed", "float");
        replaceAllString(current, "mat2", "float2x2");
        replaceAllString(current, "mat3", "float3x3");
        replaceAllString(current, "mat4", "float4x4");
        replaceAllString(current, "fract", "frac");
        replaceAllString(current, "mix", "lerp");
        if (bx::strFind(current.c_str(), "main(") != nullptr) {
            value.append(current);
            value.append("\n");
            value.append(isVertexShader ? "    VS_OUTPUT output = (VS_OUTPUT) 0;\n" : "    PS_OUTPUT output = (PS_OUTPUT) 0;\n");
            braceDepth = 1;
            continue;
        }
        for (const char *it = current.c_str(), *end = it + current.size(); it != end; ++it) {
            if (*it == '{') {
                braceDepth++;
            }
            else if (*it == '}') {
                if (braceDepth == 1 && !insertedReturn) {
                    value.append("    return output;\n");
                    insertedReturn = true;
                }
                braceDepth--;
            }
        }
        value.append(current);
        value.append("\n");
    }
}

static inline bool
findString(const bx::StringView &line, const char *outPixelCandidate, bx::StringView &p)
{
    p = bx::strFindI(line, outPixelCandidate);
    return !p.isEmpty();
}

static inline void
setSymbolRegisterIndex(const Fx9__Effect__Dx9ms__Symbol *symbolPtr, nanoem_u32_t min, nanoem_u32_t max,
    RegisterIndex &index) NANOEM_DECL_NOEXCEPT
{
    if (index.m_type == nanoem_u32_t(-1)) {
        index.m_index = symbolPtr->register_index;
        index.m_count = symbolPtr->register_count;
        index.m_type = glm::clamp(Inline::saturateInt32U(symbolPtr->register_set), min, max);
    }
}

static void
setImageTypesFromSampler(
    const Fx9__Effect__Dx9ms__Shader *shaderPtr, sg_shader_image_desc *pixelShaderSamplers) NANOEM_DECL_NOEXCEPT
{
    const size_t numSamplers = shaderPtr->n_samplers;
    for (size_t i = 0; i < numSamplers; i++) {
        const Fx9__Effect__Dx9ms__Sampler *samplerPtr = shaderPtr->samplers[i];
        const nanoem_u32_t samplerIndex = Inline::saturateInt32(samplerPtr->index);
        if (samplerIndex < SG_MAX_SHADERSTAGE_IMAGES) {
            sg_shader_image_desc &desc = pixelShaderSamplers[samplerIndex];
            desc.name = samplerPtr->name;
            switch (static_cast<Fx9__Effect__Dx9ms__SamplerType>(samplerPtr->type)) {
            case FX9__EFFECT__DX9MS__SAMPLER_TYPE__SAMPLER_2D:
            default:
                desc.image_type = SG_IMAGETYPE_2D;
                break;
            case FX9__EFFECT__DX9MS__SAMPLER_TYPE__SAMPLER_CUBE:
                desc.image_type = SG_IMAGETYPE_CUBE;
                break;
            case FX9__EFFECT__DX9MS__SAMPLER_TYPE__SAMPLER_VOLUME:
                desc.image_type = SG_IMAGETYPE_3D;
                break;
            }
        }
    }
}

static void
setRegisterIndex(const Fx9__Effect__Dx9ms__Symbol *symbolPtr, RegisterIndexMap &registerIndices)
{
    const String name(symbolPtr->name);
    RegisterIndexMap::iterator it = registerIndices.find(name);
    if (it != registerIndices.end()) {
        setSymbolRegisterIndex(symbolPtr, FX9__EFFECT__DX9MS__REGISTER_SET__RS_BOOL,
            FX9__EFFECT__DX9MS__REGISTER_SET__RS_FLOAT4, it->second);
    }
    else {
        const RegisterIndex &index = createSymbolRegisterIndex(
            symbolPtr, FX9__EFFECT__DX9MS__REGISTER_SET__RS_BOOL, FX9__EFFECT__DX9MS__REGISTER_SET__RS_FLOAT4);
        registerIndices.insert(tinystl::make_pair(name, index));
    }
}

void
rewriteShaderCode(const Fx9__Effect__Dx9ms__Shader *shaderPtr, StringMap &shaderOutputVariables, String &newShaderCode)
{
    char tempBuffer[255], variableName[64], glPrefixedVariableName[64], outPixelCandidate[64] = { 0 };
    switch (shaderPtr->type) {
    case FX9__EFFECT__DX9MS__SHADER_TYPE__ST_VERTEX: {
        const size_t numAttributes = shaderPtr->n_attributes;
        for (size_t i = 0; i < numAttributes; i++) {
            const Fx9__Effect__Dx9ms__Attribute *attributePtr = shaderPtr->attributes[i];
            switch (attributePtr->usage) {
            case FX9__EFFECT__DX9MS__ATTRIBUTE_USAGE__AU_POSITION: {
                newShaderCode.append("attribute highp vec4 a_position;\n");
                StringUtils::format(tempBuffer, sizeof(tempBuffer), "#define %s a_position\n", attributePtr->name);
                newShaderCode.append(tempBuffer);
                break;
            }
            case FX9__EFFECT__DX9MS__ATTRIBUTE_USAGE__AU_NORMAL: {
                newShaderCode.append("attribute mediump vec4 a_normal;\n");
                StringUtils::format(tempBuffer, sizeof(tempBuffer), "#define %s a_normal\n", attributePtr->name);
                newShaderCode.append(tempBuffer);
                break;
            }
            case FX9__EFFECT__DX9MS__ATTRIBUTE_USAGE__AU_TEXCOORD: {
                StringUtils::format(
                    tempBuffer, sizeof(tempBuffer), "attribute highp vec4 a_texcoord%d;\n", attributePtr->index);
                newShaderCode.append(tempBuffer);
                StringUtils::format(tempBuffer, sizeof(tempBuffer), "#define %s a_texcoord%d\n", attributePtr->name,
                    attributePtr->index);
                newShaderCode.append(tempBuffer);
                break;
            }
            case FX9__EFFECT__DX9MS__ATTRIBUTE_USAGE__AU_COLOR: {
                StringUtils::format(
                    tempBuffer, sizeof(tempBuffer), "attribute lowp vec4 a_color%d;\n", attributePtr->index);
                newShaderCode.append(tempBuffer);
                StringUtils::format(
                    tempBuffer, sizeof(tempBuffer), "#define %s a_color%d\n", attributePtr->name, attributePtr->index);
                newShaderCode.append(tempBuffer);
                break;
            }
            case FX9__EFFECT__DX9MS__ATTRIBUTE_USAGE__AU_BLENDWEIGHT: {
                newShaderCode.append("attribute highp vec4 a_weight;\n");
                StringUtils::format(tempBuffer, sizeof(tempBuffer), "#define %s a_weight\n", attributePtr->name);
                newShaderCode.append(tempBuffer);
                break;
            }
            default:
                break;
            }
        }
        const size_t numOutputs = shaderPtr->n_outputs;
        for (size_t i = 0; i < numOutputs; i++) {
            const Fx9__Effect__Dx9ms__Attribute *outputPtr = shaderPtr->outputs[i];
            StringUtils::format(tempBuffer, sizeof(tempBuffer), "varying highp vec4 %s;\n", outputPtr->name);
            switch (outputPtr->usage) {
            case FX9__EFFECT__DX9MS__ATTRIBUTE_USAGE__AU_TEXCOORD: {
                newShaderCode.append(tempBuffer);
                StringUtils::format(tempBuffer, sizeof(tempBuffer), "gl_TexCoord[%d]", outputPtr->index);
                const String name(outputPtr->name);
                shaderOutputVariables.insert(tinystl::make_pair(String(tempBuffer), name));
                if (outputPtr->index == 0) {
                    shaderOutputVariables.insert(tinystl::make_pair(String("gl_TexCoord"), name));
                }
                break;
            }
            case FX9__EFFECT__DX9MS__ATTRIBUTE_USAGE__AU_COLOR: {
                newShaderCode.append(tempBuffer);
                switch (outputPtr->index) {
                case 0: {
                    shaderOutputVariables.insert(tinystl::make_pair(String("gl_Color"), String(outputPtr->name)));
                    break;
                }
                case 1: {
                    shaderOutputVariables.insert(
                        tinystl::make_pair(String("gl_SecondaryColor"), String(outputPtr->name)));
                    break;
                }
                default:
                    break;
                }
                break;
            }
            default:
                break;
            }
        }
        break;
    }
    case FX9__EFFECT__DX9MS__SHADER_TYPE__ST_PIXEL: {
        StringSet varyings;
        for (StringMap::const_iterator it = shaderOutputVariables.begin(), end = shaderOutputVariables.end(); it != end;
             ++it) {
            const String name(it->second);
            if (varyings.find(name) == varyings.end()) {
                StringUtils::format(tempBuffer, sizeof(tempBuffer), "varying highp vec4 %s; /* %s */\n", name.c_str(),
                    it->first.c_str());
                newShaderCode.append(tempBuffer);
                varyings.insert(name);
            }
        }
        break;
    }
    default:
        return;
    }
    const bool isVertexShader = shaderPtr->type == FX9__EFFECT__DX9MS__SHADER_TYPE__ST_VERTEX;
    const bool isPixelShader = shaderPtr->type == FX9__EFFECT__DX9MS__SHADER_TYPE__ST_PIXEL;
    const size_t numUniforms = shaderPtr->n_uniforms;
    nanoem_u32_t maxUniformIndices = 0;
    for (size_t i = 0; i < numUniforms; i++) {
        const Fx9__Effect__Dx9ms__Uniform *uniformPtr = shaderPtr->uniforms[i];
        maxUniformIndices = glm::max(maxUniformIndices, uniformPtr->index);
    }
    static const char kAttributeVSVPrefix[] = "attribute vec4 vs_v";
    static const char kUniformPrefix[] = "uniform vec4 ";
    static const char kOutPixelCandidatePrefix[] = "out vec4 ";
    static const char kDefineVSCPrefix[] = "#define vs_c";
    static const char kDefineVSOPrefix[] = "#define vs_o";
    static const char kDefinePSCPrefix[] = "#define ps_c";
    static const char kDefinePSVPrefix[] = "#define ps_v";
    static const char kVersionPrefix[] = "#version ";
    bx::LineReader reader(shaderPtr->code);
    while (!reader.isDone()) {
        const bx::StringView line = reader.next();
        const char *linePtr = line.getPtr();
        bool isLineProceeded = false;
        if (StringUtils::hasPrefix(line.getPtr(), kVersionPrefix)) {
            isLineProceeded = true;
        }
        else if (StringUtils::hasPrefix(linePtr, kUniformPrefix)) {
            const bx::StringView basePtr(line, sizeof(kDefineVSCPrefix) - 1), fromPtr = bx::strLTrimSpace(basePtr),
                                                                              toPtr = bx::strWord(fromPtr);
            StringUtils::copyString(variableName, fromPtr.getPtr(),
                glm::min(Inline::saturateInt32(sizeof(variableName)), toPtr.getLength()));
            StringUtils::format(
                tempBuffer, sizeof(tempBuffer), "%s%s[%d];\n", kUniformPrefix, variableName, maxUniformIndices + 1);
            newShaderCode.append(tempBuffer);
            isLineProceeded = true;
        }
        else if (isPixelShader) {
            bx::StringView p;
            if (StringUtils::hasPrefix(linePtr, kOutPixelCandidatePrefix)) {
                const char *start = linePtr + sizeof(kOutPixelCandidatePrefix) - 1;
                const bx::StringView block = bx::strFindBlock(start, '[', ']');
                if (!block.isEmpty()) {
                    StringUtils::copyString(outPixelCandidate, block.getPtr(), block.getLength());
                }
                isLineProceeded = true;
            }
            else if (StringUtils::hasPrefix(linePtr, kDefinePSCPrefix)) {
                const bx::StringView basePtr(line, sizeof(kDefinePSCPrefix) - 1);
                char *mutablePtr = nullptr;
                int uniformIndex = StringUtils::parseInteger(basePtr.getPtr(), &mutablePtr);
                const bx::StringView fromPtr = bx::strLTrimSpace(mutablePtr), toPtr = bx::strWord(fromPtr);
                StringUtils::copyString(variableName, fromPtr.getPtr(),
                    glm::min(Inline::saturateInt32(sizeof(variableName)), toPtr.getLength()));
                StringUtils::format(tempBuffer, sizeof(tempBuffer), "%s%d %s[%d]\n", kDefinePSCPrefix, uniformIndex,
                    variableName, uniformIndex);
                newShaderCode.append(tempBuffer);
                isLineProceeded = true;
            }
            else if (StringUtils::hasPrefix(linePtr, kDefinePSVPrefix)) {
                const bx::StringView basePtr(line, sizeof(kDefinePSCPrefix) - 1);
                char *mutablePtr = nullptr;
                int variableIndex = StringUtils::parseInteger(basePtr.getPtr(), &mutablePtr);
                StringUtils::format(variableName, sizeof(variableName), "ps_v%d", variableIndex);
                const bx::StringView fromPtr = bx::strLTrimSpace(mutablePtr);
                const char *strPtr = fromPtr.getPtr();
                while (strPtr != line.getTerm() &&
                    (bx::isAlphaNum(*strPtr) || *strPtr == '_' || *strPtr == '[' || *strPtr == ']')) {
                    strPtr++;
                }
                StringUtils::copyString(
                    glPrefixedVariableName, fromPtr.getPtr(), size_t(strPtr - fromPtr.getPtr() + 1));
                StringMap::const_iterator it = shaderOutputVariables.find(glPrefixedVariableName);
                if (it != shaderOutputVariables.end()) {
                    StringUtils::format(
                        tempBuffer, sizeof(tempBuffer), "#define %s %s\n", variableName, it->second.c_str());
                    newShaderCode.append(tempBuffer);
                }
                isLineProceeded = true;
            }
            else if (*outPixelCandidate && findString(line, outPixelCandidate, p)) {
                newShaderCode.append(linePtr, p.getPtr());
                bool found = !bx::findIdentifierMatch(outPixelCandidate, "_gl_FragData").isEmpty();
                newShaderCode.append(found ? "gl_FragData" : "gl_FragColor");
                newShaderCode.append(p.getPtr() + bx::strLen(outPixelCandidate), p.getTerm());
                isLineProceeded = true;
            }
        }
        else if (isVertexShader) {
            if (StringUtils::hasPrefix(linePtr, kDefineVSCPrefix)) {
                const bx::StringView basePtr(line, sizeof(kDefinePSCPrefix) - 1);
                char *mutablePtr = nullptr;
                int uniformIndex = StringUtils::parseInteger(basePtr.getPtr(), &mutablePtr);
                const bx::StringView fromPtr = bx::strLTrimSpace(mutablePtr), toPtr = bx::strWord(fromPtr);
                StringUtils::copyString(variableName, fromPtr.getPtr(),
                    glm::min(Inline::saturateInt32(sizeof(variableName)), toPtr.getLength()));
                StringUtils::format(tempBuffer, sizeof(tempBuffer), "%s%d %s[%d]\n", kDefineVSCPrefix, uniformIndex,
                    variableName, uniformIndex);
                newShaderCode.append(tempBuffer);
                isLineProceeded = true;
            }
            else if (StringUtils::hasPrefix(linePtr, kAttributeVSVPrefix) ||
                StringUtils::hasPrefix(linePtr, kDefineVSOPrefix)) {
                newShaderCode.append("// ");
                newShaderCode.append(linePtr, line.getTerm());
                newShaderCode.append("\n");
                isLineProceeded = true;
            }
        }
        if (!isLineProceeded && !line.isEmpty() && *line.getPtr() != '\r') {
            newShaderCode.append(linePtr, line.getTerm());
            newShaderCode.append("\n");
        }
    }
}

void
createShader(const Fx9__Effect__Dx9ms__Shader *shaderPtr, sg_shader_desc &desc, String &newShaderCode,
    StringMap &shaderOutputVariables)
{
    if (shaderPtr->code && bx::strLen(shaderPtr->code) > 0) {
        rewriteShaderCode(shaderPtr, shaderOutputVariables, newShaderCode);
        switch (shaderPtr->type) {
        case FX9__EFFECT__DX9MS__SHADER_TYPE__ST_PIXEL: {
            desc.fs.source = newShaderCode.c_str();
            break;
        }
        case FX9__EFFECT__DX9MS__SHADER_TYPE__ST_VERTEX: {
            desc.vs.source = newShaderCode.c_str();
            break;
        }
        default:
            nanoem_assert(false, "must NOT reach here");
            break;
        }
    }
}

void
createHLSLShader(const Fx9__Effect__Dx9ms__Shader *shaderPtr, sg_shader_desc &desc, String &newShaderCode,
    StringMap &shaderOutputVariables)
{
    if (!shaderPtr->code || bx::strLen(shaderPtr->code) == 0) {
        return;
    }
    const bool isVertexShader = shaderPtr->type == FX9__EFFECT__DX9MS__SHADER_TYPE__ST_VERTEX;
    tinystl::vector<HLSLField, TinySTLAllocator> fields;
    appendCommonHLSLDefines(newShaderCode);
    if (isVertexShader) {
        collectVertexInputFields(shaderPtr, fields);
        newShaderCode.append("struct VS_INPUT {\n");
        for (size_t i = 0; i < fields.size(); i++) {
            appendField(newShaderCode, "float4", fields[i]);
        }
        newShaderCode.append("};\n");
        fields.clear();
        collectVertexOutputFields(shaderPtr, fields);
        newShaderCode.append("struct VS_OUTPUT {\n");
        for (size_t i = 0; i < fields.size(); i++) {
            appendField(newShaderCode, "float4", fields[i]);
        }
        newShaderCode.append("};\n");
        appendUniformBufferDeclaration(shaderPtr, "vs_uniforms_vec", newShaderCode);
        appendSamplerDeclarations(shaderPtr, newShaderCode);
        newShaderCode.append("#define vs_o0 output.position\n");
        const size_t numAttributes = shaderPtr->n_attributes;
        for (size_t i = 0; i < numAttributes; i++) {
            const Fx9__Effect__Dx9ms__Attribute *attributePtr = shaderPtr->attributes[i];
            switch (attributePtr->usage) {
            case FX9__EFFECT__DX9MS__ATTRIBUTE_USAGE__AU_POSITION:
                newShaderCode.append("#define ");
                newShaderCode.append(attributePtr->name);
                newShaderCode.append(" input.a_position\n");
                break;
            case FX9__EFFECT__DX9MS__ATTRIBUTE_USAGE__AU_NORMAL:
                newShaderCode.append("#define ");
                newShaderCode.append(attributePtr->name);
                newShaderCode.append(" input.a_normal\n");
                break;
            case FX9__EFFECT__DX9MS__ATTRIBUTE_USAGE__AU_TEXCOORD: {
                char buffer[64];
                StringUtils::format(buffer, sizeof(buffer), "#define %s input.a_texcoord%d\n", attributePtr->name,
                    attributePtr->index);
                newShaderCode.append(buffer);
                break;
            }
            case FX9__EFFECT__DX9MS__ATTRIBUTE_USAGE__AU_COLOR: {
                char buffer[64];
                StringUtils::format(buffer, sizeof(buffer), "#define %s input.a_color%d\n", attributePtr->name,
                    attributePtr->index);
                newShaderCode.append(buffer);
                break;
            }
            case FX9__EFFECT__DX9MS__ATTRIBUTE_USAGE__AU_BLENDWEIGHT:
                newShaderCode.append("#define ");
                newShaderCode.append(attributePtr->name);
                newShaderCode.append(" input.a_weight\n");
                break;
            default:
                break;
            }
        }
        for (size_t i = 0, numOutputs = shaderPtr->n_outputs; i < numOutputs; i++) {
            const Fx9__Effect__Dx9ms__Attribute *outputPtr = shaderPtr->outputs[i];
            newShaderCode.append("#define ");
            newShaderCode.append(outputPtr->name);
            newShaderCode.append(" output.");
            newShaderCode.append(outputPtr->name);
            newShaderCode.append("\n");
        }
        appendProcessedHLSLBody(shaderPtr, true, newShaderCode);
        desc.vs.source = newShaderCode.c_str();
    }
    else {
        collectPixelInputFields(shaderOutputVariables, fields);
        newShaderCode.append("struct PS_INPUT {\n");
        for (size_t i = 0; i < fields.size(); i++) {
            appendField(newShaderCode, "float4", fields[i]);
        }
        newShaderCode.append("};\n");
        newShaderCode.append("struct PS_OUTPUT {\n");
        newShaderCode.append("    float4 o_color0 : SV_Target0;\n");
        newShaderCode.append("    float4 o_color1 : SV_Target1;\n");
        newShaderCode.append("    float4 o_color2 : SV_Target2;\n");
        newShaderCode.append("    float4 o_color3 : SV_Target3;\n");
        newShaderCode.append("};\n");
        appendUniformBufferDeclaration(shaderPtr, "ps_uniforms_vec", newShaderCode);
        appendSamplerDeclarations(shaderPtr, newShaderCode);
        appendProcessedHLSLBody(shaderPtr, false, newShaderCode);
        desc.fs.source = newShaderCode.c_str();
    }
}

void
retrieveShaderSymbols(const Fx9__Effect__Dx9ms__Shader *shaderPtr, RegisterIndexMap &registerIndices,
    UniformBufferOffsetMap &uniformBufferOffsetMap)
{
    const size_t numUniforms = shaderPtr->n_uniforms;
    for (size_t i = 0; i < numUniforms; i++) {
        const Fx9__Effect__Dx9ms__Uniform *uniformPtr = shaderPtr->uniforms[i];
        uniformBufferOffsetMap.insert(tinystl::make_pair(uniformPtr->index, uniformPtr->index));
    }
    const size_t numSymbols = shaderPtr->n_symbols;
    for (size_t i = 0; i < numSymbols; i++) {
        const Fx9__Effect__Dx9ms__Symbol *symbolPtr = shaderPtr->symbols[i];
        setRegisterIndex(symbolPtr, registerIndices);
    }
}

void
retrievePixelShaderSamplers(const Fx9__Effect__Dx9ms__Pass *pass, sg_shader_image_desc *shaderSamplers,
    ImageDescriptionMap &textureDescriptions, SamplerRegisterIndexMap &shaderRegisterIndices)
{
    Fx9__Effect__Dx9ms__Texture *const *textures = pass->textures;
    const Fx9__Effect__Dx9ms__Shader *shaderPtr = pass->pixel_shader;
    const size_t numTextures = pass->n_textures, numSamplers = shaderPtr->n_samplers;
    setImageTypesFromSampler(shaderPtr, shaderSamplers);
    for (size_t i = 0; i < numTextures; i++) {
        const Fx9__Effect__Dx9ms__Texture *texturePtr = textures[i];
        const String name(texturePtr->name);
        const nanoem_u32_t samplerIndex = texturePtr->sampler_index;
        if (samplerIndex < numSamplers) {
            if (shaderRegisterIndices.find(name) == shaderRegisterIndices.end()) {
                SamplerRegisterIndex index;
                index.m_indices.push_back(samplerIndex);
                index.m_type = FX9__EFFECT__DX9MS__PARAMETER_TYPE__PT_TEXTURE;
                shaderRegisterIndices.insert(tinystl::make_pair(name, index));
            }
            if (textureDescriptions.find(name) == textureDescriptions.end()) {
                sg_image_desc desc;
                Inline::clearZeroMemory(desc);
                convertImageDescription<Fx9__Effect__Dx9ms__Texture, Fx9__Effect__Dx9ms__SamplerState>(
                    texturePtr, desc);
                if (samplerIndex < SG_MAX_SHADERSTAGE_IMAGES) {
                    desc.type = shaderSamplers[samplerIndex].image_type;
                }
                textureDescriptions.insert(tinystl::make_pair(name, desc));
            }
        }
    }
}

void
retrieveVertexShaderSamplers(const Fx9__Effect__Dx9ms__Pass *pass, sg_shader_image_desc *shaderSamplers,
    ImageDescriptionMap &textureDescriptions, SamplerRegisterIndexMap &shaderRegisterIndices)
{
    Fx9__Effect__Dx9ms__Texture *const *textures = pass->vertex_textures;
    const Fx9__Effect__Dx9ms__Shader *shaderPtr = pass->vertex_shader;
    const size_t numTextures = pass->n_vertex_textures, numSamplers = shaderPtr->n_samplers;
    setImageTypesFromSampler(shaderPtr, shaderSamplers);
    for (size_t i = 0; i < numTextures; i++) {
        const Fx9__Effect__Dx9ms__Texture *texturePtr = textures[i];
        const String name(texturePtr->name);
        const nanoem_u32_t samplerIndex = texturePtr->sampler_index;
        if (samplerIndex < numSamplers) {
            if (shaderRegisterIndices.find(name) == shaderRegisterIndices.end()) {
                SamplerRegisterIndex index;
                index.m_indices.push_back(samplerIndex);
                index.m_type = FX9__EFFECT__DX9MS__PARAMETER_TYPE__PT_TEXTURE;
                shaderRegisterIndices.insert(tinystl::make_pair(name, index));
            }
            if (textureDescriptions.find(name) == textureDescriptions.end()) {
                sg_image_desc desc;
                Inline::clearZeroMemory(desc);
                convertImageDescription<Fx9__Effect__Dx9ms__Texture, Fx9__Effect__Dx9ms__SamplerState>(
                    texturePtr, desc);
                if (samplerIndex < SG_MAX_SHADERSTAGE_IMAGES) {
                    desc.type = shaderSamplers[samplerIndex].image_type;
                }
                textureDescriptions.insert(tinystl::make_pair(name, desc));
            }
        }
    }
}

void
parsePreshader(const Fx9__Effect__Dx9ms__Shader *shaderPtr, Preshader &preshader, GlobalUniform::Buffer &buffer,
    RegisterIndexMap &registerIndices)
{
    if (const Fx9__Effect__Dx9ms__Preshader *preshaderPtr = shaderPtr->preshader) {
        const size_t numInstructions = preshaderPtr->n_instructions;
        preshader.m_numTemporaryRegisters = preshaderPtr->num_temporary_registers;
        preshader.m_instructions.resize(numInstructions);
        for (size_t i = 0; i < numInstructions; i++) {
            const Fx9__Effect__Dx9ms__Instruction *instructionPtr = preshaderPtr->instructions[i];
            const nanoem_u32_t numOperands = Inline::saturateInt32U(instructionPtr->n_operands);
            Preshader::Instruction &instruction = preshader.m_instructions[i];
            instruction.m_numElements = glm::clamp(instructionPtr->num_elements, 0u, 4u);
            instruction.m_opcode = instructionPtr->opcode;
            instruction.m_operands.resize(numOperands);
            for (nanoem_u32_t j = 0; j < numOperands; j++) {
                const Fx9__Effect__Dx9ms__Operand *operandPtr = instructionPtr->operands[j];
                Preshader::Operand &operand = instruction.m_operands[j];
                operand.m_index = operandPtr->index;
                operand.m_type = operandPtr->type;
            }
        }
        const size_t numSymbols = preshaderPtr->n_symbols;
        preshader.m_symbols.resize(numSymbols);
        for (size_t i = 0; i < numSymbols; i++) {
            const Fx9__Effect__Dx9ms__Symbol *symbolPtr = preshaderPtr->symbols[i];
            Preshader::Symbol &symbol = preshader.m_symbols[i];
            symbol.m_name = symbolPtr->name;
            symbol.m_count = symbolPtr->register_count;
            symbol.m_index = symbolPtr->register_index;
            symbol.m_set = symbolPtr->register_set;
            setRegisterIndex(symbolPtr, registerIndices);
        }
        countRegisterSet(preshaderPtr->symbols, preshaderPtr->n_symbols, buffer);
        const size_t numLiterals = preshaderPtr->n_literals;
        preshader.m_literals.resize(numLiterals);
        for (size_t i = 0; i < numLiterals; i++) {
            nanoem_f64_t literal = preshaderPtr->literals[i];
            preshader.m_literals[i] = nanoem_f32_t(literal);
        }
    }
}

} /* namespace dx9ms */
} /* namespace effect */
} /* namespace nanoem */

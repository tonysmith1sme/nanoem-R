/*
   Copyright (c) 2015-2023 hkrn All rights reserved

   This file is licensed under MIT license. for more details, see LICENSE.txt.
 */

#include "fx9/EffectSourcePipeline.h"

#include <cctype>
#include <cstdio>
#include <cstring>
#include <regex>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

namespace fx9 {
namespace translation {
namespace {

/* -------------------------------------------------------------------------- */
/* Path helpers                                                               */
/* -------------------------------------------------------------------------- */

std::string
toLowerASCII(const std::string &value)
{
    std::string normalized(value);
    for (size_t i = 0, numChars = normalized.size(); i < numChars; i++) {
        normalized[i] = static_cast<char>(tolower(static_cast<unsigned char>(normalized[i])));
    }
    return normalized;
}

std::string
normalizePathSeparators(const std::string &path)
{
    std::string normalized(toLowerASCII(path));
    for (size_t i = 0, numChars = normalized.size(); i < numChars; i++) {
        if (normalized[i] == '\\') {
            normalized[i] = '/';
        }
    }
    return normalized;
}

bool
pathContains(const std::string &path, const char *needle)
{
    return toLowerASCII(path).find(needle) != std::string::npos;
}

bool
endsWithIgnoreCase(const std::string &value, const char *suffix)
{
    const std::string valueLC(toLowerASCII(value)), suffixLC(toLowerASCII(std::string(suffix)));
    return valueLC.size() >= suffixLC.size() &&
        valueLC.compare(valueLC.size() - suffixLC.size(), suffixLC.size(), suffixLC) == 0;
}

bool
isRayMMDPath(const std::string &path)
{
    return pathContains(path, "ray-mmd");
}

bool
isRayMMDMainEffectSourcePath(const std::string &path)
{
    const std::string normalized(normalizePathSeparators(path));
    return normalized.find("ray-mmd") != std::string::npos && normalized.find("/main/main.fxsub") != std::string::npos;
}

bool
isRayMMDShaderSourcePath(const std::string &path)
{
    const std::string normalized(normalizePathSeparators(path));
    return normalized.find("ray-mmd") != std::string::npos && normalized.find("/shader/") != std::string::npos;
}

void
replaceAll(std::string &value, const std::string &from, const std::string &to)
{
    size_t offset = 0;
    while ((offset = value.find(from, offset)) != std::string::npos) {
        value.replace(offset, from.size(), to);
        offset += to.size();
    }
}

void
replaceExact(std::string &value, const std::string &needle, const std::string &replacement)
{
    for (size_t pos = 0; (pos = value.find(needle, pos)) != std::string::npos; pos += replacement.size()) {
        value.replace(pos, needle.size(), replacement);
    }
}

/* -------------------------------------------------------------------------- */
/* Stage 1 – SourceNormalizer                                                 */
/*                                                                            */
/* Encoding residues from Shift-JIS MME sources, string-literal escapes, and  */
/* #define hygiene that Lemon/glslang cannot recover from on their own.       */
/* -------------------------------------------------------------------------- */

bool
isValidHlslStringEscapeFollower(char c)
{
    return c == '\\' || c == '"' || c == '\'' || c == '?' || c == 'a' || c == 'b' || c == 'f' || c == 'n' || c == 'r' ||
        c == 't' || c == 'v' || c == 'x' || c == 'u' || (c >= '0' && c <= '7');
}

std::string
normalizeYenSignLineContinuations(const std::string &source)
{
    /* U+00A5 (UTF-8 C2 A5) is often used as line-continuation in SJIS-era MME. */
    std::string fixed;
    fixed.reserve(source.size());
    for (size_t i = 0; i < source.size(); i++) {
        if (static_cast<unsigned char>(source[i]) == 0xC2 && i + 1 < source.size() &&
            static_cast<unsigned char>(source[i + 1]) == 0xA5) {
            fixed.push_back('\\');
            i++;
        }
        else {
            fixed.push_back(source[i]);
        }
    }
    return fixed;
}

std::string
normalizeStrayBackslashesInStringLiterals(const std::string &source)
{
    /* SJIS multi-byte sequences can leave a lone 0x5c inside UTF-8 string
       literals ("AO表\示"). Duplicate any `\` that is not a valid HLSL escape. */
    std::string out;
    out.reserve(source.size() + 16);
    const size_t n = source.size();
    size_t i = 0;
    while (i < n) {
        char c = source[i];
        if (c != '"') {
            out.push_back(c);
            i++;
            continue;
        }
        out.push_back(c);
        i++;
        while (i < n) {
            char d = source[i];
            if (d == '\n' || d == '\r') {
                while (i < n) {
                    out.push_back(source[i++]);
                }
                return out;
            }
            if (d == '\\') {
                if (i + 1 < n && isValidHlslStringEscapeFollower(source[i + 1])) {
                    out.push_back('\\');
                    out.push_back(source[i + 1]);
                    i += 2;
                    continue;
                }
                out.push_back('\\');
                out.push_back('\\');
                i++;
                continue;
            }
            if (d == '"') {
                out.push_back(d);
                i++;
                break;
            }
            out.push_back(d);
            i++;
        }
    }
    return out;
}

std::string
trimASCII(const std::string &value)
{
    size_t begin = 0, end = value.size();
    while (begin < end && isspace(static_cast<unsigned char>(value[begin]))) {
        begin++;
    }
    while (end > begin && isspace(static_cast<unsigned char>(value[end - 1]))) {
        end--;
    }
    return value.substr(begin, end - begin);
}

bool
parseMacroDirective(const std::string &line, const char *directive, std::string &name, std::string &substitution)
{
    size_t offset = 0;
    while (offset < line.size() && isspace(static_cast<unsigned char>(line[offset]))) {
        offset++;
    }
    if (offset >= line.size() || line[offset] != '#') {
        return false;
    }
    offset++;
    while (offset < line.size() && isspace(static_cast<unsigned char>(line[offset]))) {
        offset++;
    }
    const size_t directiveLength = strlen(directive);
    if (line.compare(offset, directiveLength, directive) != 0) {
        return false;
    }
    offset += directiveLength;
    if (offset < line.size() && !isspace(static_cast<unsigned char>(line[offset]))) {
        return false;
    }
    while (offset < line.size() && isspace(static_cast<unsigned char>(line[offset]))) {
        offset++;
    }
    const size_t nameOffset = offset;
    if (offset < line.size() && (isalpha(static_cast<unsigned char>(line[offset])) || line[offset] == '_')) {
        offset++;
        while (offset < line.size()) {
            const unsigned char c = static_cast<unsigned char>(line[offset]);
            if (!(isalnum(c) || c == '_')) {
                break;
            }
            offset++;
        }
        name.assign(line, nameOffset, offset - nameOffset);
        substitution = trimASCII(line.substr(offset));
        return !name.empty();
    }
    return false;
}

std::string
normalizeRedefineMacros(const std::string &source)
{
    /* Insert #undef before a #define that redefines an existing macro with a
       different substitution so glslang does not abort on redefinition. */
    std::unordered_map<std::string, std::string> macroSubstitutions;
    std::string result;
    result.reserve(source.size() + 64);
    size_t offset = 0;
    while (offset <= source.size()) {
        const size_t end = source.find('\n', offset);
        const bool hasLineFeed = end != std::string::npos;
        const std::string line(hasLineFeed ? source.substr(offset, end - offset) : source.substr(offset));
        std::string name, substitution;
        if (parseMacroDirective(line, "undef", name, substitution)) {
            macroSubstitutions.erase(name);
        }
        else if (parseMacroDirective(line, "define", name, substitution)) {
            const auto it = macroSubstitutions.find(name);
            if (it != macroSubstitutions.end() && it->second != substitution) {
                result.append("#undef ");
                result.append(name);
                result.push_back('\n');
            }
            macroSubstitutions[name] = substitution;
        }
        result.append(line);
        if (hasLineFeed) {
            result.push_back('\n');
            offset = end + 1;
        }
        else {
            break;
        }
    }
    return result;
}

std::string
runSourceNormalizer(const std::string &source)
{
    std::string stage = normalizeYenSignLineContinuations(source);
    stage = normalizeStrayBackslashesInStringLiterals(stage);
    return stage;
}

/* -------------------------------------------------------------------------- */
/* Stage 2 – LegacyEffectRules                                                */
/*                                                                            */
/* Expand constructs that Lemon's DX Effect grammar cannot accept, such as    */
/* multi-line DefTech(...) macros used by older edge/object effects.          */
/* -------------------------------------------------------------------------- */

std::string
expandDefTechMacros(const std::string &source)
{
    if (source.find("DefTech(") == std::string::npos || source.find("#define DefTech(") == std::string::npos) {
        return source;
    }
    std::string patched(source);
    bool isNormalDraw = patched.find("Subset=EdgeMaterial") != std::string::npos;
    size_t macroPos = patched.find("#define DefTech(");
    size_t scanPos = patched.find('\n', macroPos);
    if (scanPos != std::string::npos) {
        scanPos++;
        while (scanPos < patched.size()) {
            size_t nl = patched.find('\n', scanPos);
            if (nl == std::string::npos) {
                break;
            }
            size_t endChar = nl;
            while (endChar > scanPos &&
                (patched[endChar - 1] == '\r' || patched[endChar - 1] == ' ' || patched[endChar - 1] == '\t')) {
                endChar--;
            }
            bool isContinuation = false;
            if (endChar > scanPos && patched[endChar - 1] == '\\') {
                isContinuation = true;
            }
            else if (endChar >= 2 + scanPos && static_cast<unsigned char>(patched[endChar - 2]) == 0xC2 &&
                static_cast<unsigned char>(patched[endChar - 1]) == 0xA5) {
                isContinuation = true;
            }
            if (isContinuation) {
                scanPos = nl + 1;
            }
            else {
                scanPos = nl + 1;
                break;
            }
        }
        patched.erase(macroPos, scanPos - macroPos);
    }
    const char *defTechArgs[] = { "_0, object , true, Tex)", "_1, object , false, NoTex)", "_2, object_ss , true, Tex)",
        "_3, object_ss , false, NoTex)" };
    for (size_t pos = 0; (pos = patched.find("DefTech(", pos)) != std::string::npos;) {
        size_t end = patched.find('\n', pos);
        if (end == std::string::npos) {
            end = patched.size();
        }
        std::string line = patched.substr(pos, end - pos);
        bool matched = false;
        for (size_t i = 0; i < 4 && !matched; i++) {
            if (line.find(std::string("DefTech(") + defTechArgs[i]) == 0) {
                const char *passName = (i >= 2) ? "object_ss" : "object";
                const char *useTex = (i % 2 == 0) ? "true" : "false";
                const char *texFunc = (i % 2 == 0) ? "Object_Tex_PS" : "Object_NoTex_PS";
                char buf[1024];
                if (isNormalDraw) {
                    snprintf(buf, sizeof(buf),
                        "technique ObjectEdgeTec_%zu < string MMDPass = \"%s\"; string Subset=EdgeMaterial; bool "
                        "UseTexture=%s;> { pass DrawEdge { AlphaBlendEnable = FALSE; AlphaTestEnable = FALSE; "
                        "VertexShader = compile vs_2_0 Object_VS(); PixelShader = compile ps_2_0 %s(1); } } technique "
                        "ObjectNoEdgeTec_%zu < string MMDPass = \"%s\"; bool UseTexture=%s;> { pass DrawEdge { "
                        "AlphaBlendEnable = FALSE; AlphaTestEnable = FALSE; VertexShader = compile vs_2_0 Object_VS(); "
                        "PixelShader = compile ps_2_0 %s(0); } }",
                        i, passName, useTex, texFunc, i, passName, useTex, texFunc);
                }
                else {
                    snprintf(buf, sizeof(buf),
                        "technique ObjectTec_%zu < string MMDPass = \"%s\"; bool UseTexture=%s;> { pass DrawEdge { "
                        "AlphaBlendEnable = true; VertexShader = compile vs_2_0 Object_VS(); PixelShader = compile "
                        "ps_2_0 %s(); } }",
                        i, passName, useTex, texFunc);
                }
                patched.replace(pos, line.size(), buf);
                pos += strlen(buf);
                matched = true;
            }
        }
        if (!matched) {
            pos = end + 1;
        }
    }
    return patched;
}

std::string
runLegacyEffectRules(const std::string &source)
{
    return expandDefTechMacros(source);
}

/* -------------------------------------------------------------------------- */
/* Stage 1 (pipeline order) – CompatibilityProfiles                            */
/*                                                                            */
/* Profile-driven rewrites for known large effects (primarily ray-mmd) where  */
/* DX9 HLSL constructs or Metal/HLSL backend limits require IR-safe source    */
/* adjustments before glslang. Rules are keyed by path suffix / profile, not  */
/* free-form global regex.                                                    */
/* -------------------------------------------------------------------------- */

std::string
replaceRayMMDIntegerDefine(
    const std::string &source, const char *name, int value, const std::regex_constants::syntax_option_type flags)
{
    std::ostringstream pattern, replacement;
    pattern << "(^|\\n)([ \\t]*#define[ \\t]+" << name << "[ \\t]+)[0-9]+([ \\t]*(?://[^\\n]*)?)";
    replacement << "$1$2" << value << "$3";
    return std::regex_replace(source, std::regex(pattern.str(), flags), replacement.str());
}

std::string
applyRayMMDMetalHeavySamplingRules(const std::string &path, const std::string &source)
{
    if (!isRayMMDShaderSourcePath(path)) {
        return source;
    }
    std::string patched(source);
    patched = std::regex_replace(patched, std::regex(R"(\[\s*unroll\s*\])", std::regex_constants::icase), "[loop]");
    patched = std::regex_replace(patched,
        std::regex(R"(#\s*define\s+SSDO_UNROLL\s+\[\s*unroll\s*\])", std::regex_constants::icase),
        "#define SSDO_UNROLL [loop]");
    patched = std::regex_replace(patched,
        std::regex(R"(#\s*define\s+SSDO_UNROLL\s*(?:\r?\n))", std::regex_constants::icase),
        "#define SSDO_UNROLL [loop]\n");
    patched = std::regex_replace(patched,
        std::regex(R"(#\s*define\s+SSR_SAMPLER_COUNT\s+64\b)", std::regex_constants::icase),
        "#define SSR_SAMPLER_COUNT 48");
    patched = std::regex_replace(patched,
        std::regex(R"(#\s*define\s+SSR_SAMPLER_COUNT\s+128\b)", std::regex_constants::icase),
        "#define SSR_SAMPLER_COUNT 64");
    return patched;
}

std::string
injectRayMMDConfigurationOverride(const std::string &source)
{
    /* Quality caps for Metal/HLSL backends after ray.conf is included.
       Kept as preprocessor so one source tree still works on DX9 MME. */
    static const char *kRayMMDBackendConfigurationOverride =
        "\n#if NANOEM_OUTPUT_SHADER_LANGUAGE_MSL\n"
        "#undef SUN_SHADOW_QUALITY\n"
        "#define SUN_SHADOW_QUALITY 3\n"
        "#undef IBL_QUALITY\n"
        "#define IBL_QUALITY 1\n"
        "#undef MULTI_LIGHT_ENABLE\n"
        "#define MULTI_LIGHT_ENABLE 0\n"
        "#undef SSDO_QUALITY\n"
        "#define SSDO_QUALITY 0\n"
        "#undef SSR_QUALITY\n"
        "#define SSR_QUALITY 0\n"
        "#undef SSSS_QUALITY\n"
        "#define SSSS_QUALITY 0\n"
        "#undef HDR_BLOOM_MODE\n"
        "#define HDR_BLOOM_MODE 0\n"
        "#undef OUTLINE_QUALITY\n"
        "#define OUTLINE_QUALITY 1\n"
        "#endif\n"
        "\n#if NANOEM_OUTPUT_SHADER_LANGUAGE_HLSL\n"
        "#undef SUN_SHADOW_QUALITY\n"
        "#define SUN_SHADOW_QUALITY 3\n"
        "#undef IBL_QUALITY\n"
        "#define IBL_QUALITY 1\n"
        "#undef MULTI_LIGHT_ENABLE\n"
        "#define MULTI_LIGHT_ENABLE 0\n"
        "#undef SSDO_QUALITY\n"
        "#define SSDO_QUALITY 0\n"
        "#undef SSR_QUALITY\n"
        "#define SSR_QUALITY 0\n"
        "#undef SSSS_QUALITY\n"
        "#define SSSS_QUALITY 0\n"
        "#undef HDR_BLOOM_MODE\n"
        "#define HDR_BLOOM_MODE 0\n"
        "#endif\n";
    return std::regex_replace(source, std::regex(R"((#\s*include\s+["<]ray\.conf[">]\s*))", std::regex_constants::icase),
        std::string("$1") + kRayMMDBackendConfigurationOverride);
}

std::string
rewriteGbufferStructReturnToMRT(std::string patched, bool useWindowsNewlines)
{
    /* glslang/SPIR-V cannot reliably lower struct-return MRT (GbufferParam)
       used by ray-mmd material shaders. Expand to explicit COLOR0..3 outs. */
    if (useWindowsNewlines) {
        replaceAll(patched, "\r\n", "\n");
    }
    const char *patterns[][2] = {
        { "GbufferParam MaterialPS(\n\tin float3 normal   : TEXCOORD0,\n\tin float2 coord0   : TEXCOORD1,\n\tin float4 "
          "worldPos : TEXCOORD3)",
            "void MaterialPS(\n\tin float3 normal   : TEXCOORD0,\n\tin float2 coord0   : TEXCOORD1,\n\tin float4 "
            "worldPos : TEXCOORD3, out float4 oColor0 : COLOR0, out float4 oColor1 : COLOR1, out float4 oColor2 : "
            "COLOR2, out float4 oColor3 : COLOR3)" },
        { "GbufferParam MaterialPS(\n\tin float3 normal   : TEXCOORD0,\n\tin float2 coord0   : TEXCOORD1,\n#if "
          "OCCLUSION_MAP_TYPE == 2 || OCCLUSION_MAP_TYPE == 3\n\tin float2 coord1   : TEXCOORD2,\n#endif\n\tin float4 "
          "worldPos : TEXCOORD3)",
            "void MaterialPS(\n\tin float3 normal   : TEXCOORD0,\n\tin float2 coord0   : TEXCOORD1,\n#if "
            "OCCLUSION_MAP_TYPE == 2 || OCCLUSION_MAP_TYPE == 3\n\tin float2 coord1   : TEXCOORD2,\n#endif\n\tin float4 "
            "worldPos : TEXCOORD3, out float4 oColor0 : COLOR0, out float4 oColor1 : COLOR1, out float4 oColor2 : "
            "COLOR2, out float4 oColor3 : COLOR3)" },
        { "GbufferParam Material2PS(\n\tin float3 normal   : TEXCOORD0,\n\tin float2 coord0   : TEXCOORD1,\n\tin "
          "float4 worldPos : TEXCOORD3)",
            "void Material2PS(\n\tin float3 normal   : TEXCOORD0,\n\tin float2 coord0   : TEXCOORD1,\n\tin float4 "
            "worldPos : TEXCOORD3, out float4 oColor0 : COLOR0, out float4 oColor1 : COLOR1, out float4 oColor2 : "
            "COLOR2, out float4 oColor3 : COLOR3)" },
        { "GbufferParam Material2PS(\n\tin float3 normal   : TEXCOORD0,\n\tin float2 coord0   : TEXCOORD1,\n#if "
          "OCCLUSION_MAP_TYPE == 2 || OCCLUSION_MAP_TYPE == 3\n\tin float2 coord1   : TEXCOORD2,\n#endif\n\tin float4 "
          "worldPos : TEXCOORD3)",
            "void Material2PS(\n\tin float3 normal   : TEXCOORD0,\n\tin float2 coord0   : TEXCOORD1,\n#if "
            "OCCLUSION_MAP_TYPE == 2 || OCCLUSION_MAP_TYPE == 3\n\tin float2 coord1   : TEXCOORD2,\n#endif\n\tin float4 "
            "worldPos : TEXCOORD3, out float4 oColor0 : COLOR0, out float4 oColor1 : COLOR1, out float4 oColor2 : "
            "COLOR2, out float4 oColor3 : COLOR3)" },
    };
    if (useWindowsNewlines) {
        for (const auto &pair : patterns) {
            replaceAll(patched, pair[0], pair[1]);
        }
        replaceAll(patched, "\treturn EncodeGbuffer(material, worldPos.w);",
            "\tGbufferParam __gb = EncodeGbuffer(material, worldPos.w);\n\toColor0 = __gb.buffer1; oColor1 = "
            "__gb.buffer2; oColor2 = __gb.buffer3; oColor3 = __gb.buffer4;");
    }
    return patched;
}

std::string
applyRayMMDCompatibilityRules(const std::string &path, const std::string &source)
{
    if (!isRayMMDPath(path)) {
        return source;
    }
    std::string patched(source);
    if (endsWithIgnoreCase(path, "ray.conf")) {
        const std::regex_constants::syntax_option_type flags =
            std::regex_constants::ECMAScript | std::regex_constants::icase;
        patched = replaceRayMMDIntegerDefine(patched, "OUTLINE_QUALITY", 1, flags);
    }
    if (endsWithIgnoreCase(path, "PostProcessDiffusion.fxsub")) {
        patched = std::string("#if !defined(SSDO_QUALITY) || SSDO_QUALITY == 0\n"
                              "float linearizeDepth(float2 uv) { return tex2Dlod(Gbuffer8Map, float4(uv, 0, 0)).r; }\n"
                              "#endif\n") +
            patched;
    }
    if (endsWithIgnoreCase(path, "ShadingMaterials.fxsub")) {
        /* Use needle.size() for exact replacement — hardcoded lengths previously
           truncated the following "specular" token into "pecular". */
        replaceExact(patched, "diffuse += tex2Dlod(LightMapSamp, float4(coord, 0, 0)).rgb;",
            "diffuse += max(tex2Dlod(LightMapSamp, float4(coord, 0, 0)).rgb, 0.0);");
        replaceExact(patched, "specular += tex2Dlod(LightSpecMapSamp, float4(coord, 0, 0)).rgb;",
            "specular += max(tex2Dlod(LightSpecMapSamp, float4(coord, 0, 0)).rgb, 0.0);");
        patched = std::regex_replace(patched, std::regex(R"(diffuse \+= iblDiffuse;)"),
            "diffuse += max(0.0, min(iblDiffuse, 1e10));");
        patched = std::regex_replace(patched, std::regex(R"(specular \+= iblSpecular;)"),
            "specular += max(0.0, min(iblSpecular, 1e10));");
        patched = std::regex_replace(patched,
            std::regex("(oColor0 = float4\\(diffuse \\* material\\.albedo \\+ specular, material\\.linearDepth\\);)"),
            "diffuse = max(diffuse, max(material.albedo, 0.02) * 0.08);\n\t"
            "diffuse += material.albedo * 0.04;\n\t"
            "specular = max(specular, float3(0.005, 0.005, 0.005));\n\t$1");
        /* Metal YCbCr path misaligns channels; read packed RGB with clamp. */
        patched = std::regex_replace(patched,
            std::regex(
                R"(DecodeYcbcr\(source\s*,\s*coord\s*,\s*screenPosition\s*,\s*ViewportOffset2\s*,\s*diffuse\s*,\s*specular\s*\)\s*;)"),
            "{ float4 __ibl = tex2Dlod(source, float4(coord, 0, 0)); "
            "float3 __iblRgb = clamp(__ibl.rgb, 0.0, 8.0); "
            "if (any(isnan(__ibl.rgb)) || any(isinf(__ibl.rgb))) __iblRgb = 0.0; "
            "diffuse = __iblRgb; specular = __iblRgb; }");
    }
    if (endsWithIgnoreCase(path, "Sky with lighting.fx")) {
        patched = std::regex_replace(patched, std::regex("(#include\\s+\"[^\"]+\"\\s*)"),
            "#define MIDPOINT_8_BIT (127.0f / 255.0f)\n$1", std::regex_constants::format_first_only);
        patched = std::regex_replace(patched,
            std::regex(R"(oColor0\s*=\s*EncodeYcbcr\s*\(\s*screenPosition\s*,\s*diffuse\s*,\s*specular\s*\)\s*;)"),
            "oColor0 = float4(diffuse, 0);");
        patched = std::regex_replace(patched,
            std::regex(R"(oColor1\s*=\s*EncodeYcbcr\s*\(\s*screenPosition\s*,\s*diffuse2\s*,\s*specular2\s*\)\s*;)"),
            "oColor1 = float4(specular, 0);");
    }
    if (endsWithIgnoreCase(path, "skylighting.fxsub") || endsWithIgnoreCase(path, "skylighting_fast.fxsub")) {
        patched = std::regex_replace(patched,
            std::regex(R"(oColor0\s*=\s*EncodeYcbcr\s*\(\s*screenPosition\s*,\s*diffuse\s*,\s*specular\s*\)\s*;)"),
            "oColor0 = float4(diffuse, 0);");
        patched = std::regex_replace(patched,
            std::regex(R"(oColor1\s*=\s*EncodeYcbcr\s*\(\s*screenPosition\s*,\s*diffuse2\s*,\s*specular2\s*\)\s*;)"),
            "oColor1 = float4(specular, 0);");
        patched = std::regex_replace(patched,
            std::regex(R"(oColor0\s*=\s*EncodeYcbcr\s*\(\s*screenPosition\s*,\s*0\s*,\s*0\s*\)\s*;)"),
            "oColor0 = float4(0, 0, 0, 0);");
        patched = std::regex_replace(patched,
            std::regex(R"(oColor1\s*=\s*EncodeYcbcr\s*\(\s*screenPosition\s*,\s*0\s*,\s*0\s*\)\s*;)"),
            "oColor1 = float4(0, 0, 0, 0);");
    }
    if (endsWithIgnoreCase(path, "material_common.fxsub")) {
        patched = std::regex_replace(patched,
            std::regex("GbufferParam MaterialPS\\("
                       "\\s*in float3 normal\\s*:\\s*TEXCOORD0\\s*,"
                       "\\s*in float2 coord\\s*:\\s*TEXCOORD1\\s*,"
                       "\\s*in float4 worldPos\\s*:\\s*TEXCOORD2\\s*,"
                       "\\s*in float4 viewdir\\s*:\\s*TEXCOORD3\\s*\\)"),
            "void MaterialPS(in float3 normal : TEXCOORD0, in float2 coord : TEXCOORD1, in float4 worldPos : "
            "TEXCOORD2, in float4 viewdir : TEXCOORD3, out float4 oColor0 : COLOR0, out float4 oColor1 : COLOR1, out "
            "float4 oColor2 : COLOR2, out float4 oColor3 : COLOR3)");
        patched = std::regex_replace(patched,
            std::regex("GbufferParam MaterialPS\\("
                       "\\s*in float3 normal\\s*:\\s*TEXCOORD0\\s*,"
                       "\\s*in float2 coord\\s*:\\s*TEXCOORD1\\s*,"
                       "\\s*in float4 worldPos\\s*:\\s*TEXCOORD2\\s*\\)"),
            "void MaterialPS(in float3 normal : TEXCOORD0, in float2 coord : TEXCOORD1, in float4 worldPos : "
            "TEXCOORD2, out float4 oColor0 : COLOR0, out float4 oColor1 : COLOR1, out float4 oColor2 : COLOR2, out "
            "float4 oColor3 : COLOR3)");
        patched = std::regex_replace(patched,
            std::regex("GbufferParam Material2PS\\("
                       "\\s*in float3 normal\\s*:\\s*TEXCOORD0\\s*,"
                       "\\s*in float2 coord\\s*:\\s*TEXCOORD1\\s*,"
                       "\\s*in float4 worldPos\\s*:\\s*TEXCOORD2\\s*\\)"),
            "void Material2PS(in float3 normal : TEXCOORD0, in float2 coord : TEXCOORD1, in float4 worldPos : "
            "TEXCOORD2, out float4 oColor0 : COLOR0, out float4 oColor1 : COLOR1, out float4 oColor2 : COLOR2, out "
            "float4 oColor3 : COLOR3)");
        patched = std::regex_replace(patched, std::regex("return EncodeGbuffer\\(material, (\\w+)\\.w\\);"),
            "GbufferParam __gb = EncodeGbuffer(material, $1.w); "
            "oColor0 = __gb.buffer1; oColor1 = __gb.buffer2; "
            "oColor2 = __gb.buffer3; oColor3 = __gb.buffer4;");
    }
    if (endsWithIgnoreCase(path, "_2.0.fxsub")) {
        patched = rewriteGbufferStructReturnToMRT(patched, true);
    }
    if (endsWithIgnoreCase(path, "PostProcessHDR.fxsub") || endsWithIgnoreCase(path, "LightBloom.fxsub")) {
        patched = std::regex_replace(patched, std::regex(R"(\bsampler\b\s+(\w+)\s*,)"), "sampler2D $1,");
        patched = std::regex_replace(patched, std::regex(R"(\bsampler\b\s+(\w+)\s*\))"), "sampler2D $1)");
    }
    if (endsWithIgnoreCase(path, "ShadowMap.fxsub")) {
        replaceAll(patched, "SHADOW_BLUR_COUNT 6", "SHADOW_BLUR_COUNT 24");
        replaceAll(patched, "float radius = 2.0 / SHADOW_MAP_SIZE", "float radius = 48.0 / SHADOW_MAP_SIZE");
        replaceAll(patched, "exp(-20 *", "exp(-2 *");
        replaceAll(patched, "float2 offset1 = coord + offset", "float2 offset1 = coord + offset * 8");
        replaceAll(patched, "float2 offset2 = coord - offset", "float2 offset2 = coord - offset * 8");
        replaceAll(patched, "BilateralWeight(r, depth1, center_d, SHADOW_BLUR_COUNT, 10)",
            "BilateralWeight(r, depth1, center_d, 10, 10)");
        replaceAll(patched, "BilateralWeight(r, depth2, center_d, SHADOW_BLUR_COUNT, 10)",
            "BilateralWeight(r, depth2, center_d, 10, 10)");
    }
    if (endsWithIgnoreCase(path, "FXAA3.fxsub")) {
        patched = std::regex_replace(patched, std::regex("\\}(FxaaFloat4 FxaaPixelShader)"), "}\n$1");
        patched = std::regex_replace(patched, std::regex("(\\{)(FxaaFloat2 posM)"), "{\n$2");
        patched = std::regex_replace(patched, std::regex(";(FxaaFloat lumaM)"), ";\n$1");
        patched = std::regex_replace(patched, std::regex(";(FxaaBool earlyExit)"), ";\n$1");
        patched = std::regex_replace(patched, std::regex(";(FxaaFloat lumaNW)"), ";\n$1");
    }
    patched = injectRayMMDConfigurationOverride(patched);
    patched =
        std::regex_replace(patched, std::regex("Sky\\*box\\*\\.\\*", std::regex_constants::icase), "sky*box*.*");
    /* glslang rejects 2D array uniforms of this form; flatten to 1D. */
    patched = std::regex_replace(patched,
        std::regex(R"(float2\s+SHKernel\s*\[\s*6\s*\]\s*\[\s*9\s*\])", std::regex_constants::icase),
        "float2 SHKernel[54]");
    patched = std::regex_replace(patched,
        std::regex(R"(SHKernel\s*\[\s*([^\]]+)\s*\]\s*\[\s*([^\]]+)\s*\])", std::regex_constants::icase),
        "SHKernel[(($1) * 9) + ($2)]");
    if (isRayMMDMainEffectSourcePath(path)) {
        patched = std::regex_replace(patched,
            std::regex(
                R"(return\s+float4\s*\(\s*GetSpecularHighlight\s*\(\s*normal\s*,\s*coord\s*\)\s*,\s*0\s*\)\s*;)"),
            "return float4(MaterialDiffuse.rgb * alpha, alpha);");
    }
    patched = applyRayMMDMetalHeavySamplingRules(path, patched);
    return patched;
}

std::string
runCompatibilityRules(const std::string &path, const std::string &source)
{
    return applyRayMMDCompatibilityRules(path, source);
}

} /* namespace anonymous */

std::string
prepareEffectSource(const std::string &path, const std::string &source)
{
    /* Ordered pipeline:
         1) Compatibility profile rules (path-keyed, may inject macros)
         2) Source normalizer (encoding / escapes)
         3) Legacy effect construct expansion (DefTech)
         4) Macro redefinition hygiene (must run last so injected #defines are clean)
     Each stage reports when it rewrites the source (FX9_LOG_SOURCE_SHIMS=1) so applied
     compatibility shims stay observable instead of silently morphing effect sources. */
    static const bool reportShims = [] {
        const char *value = getenv("FX9_LOG_SOURCE_SHIMS");
        return value != nullptr && *value != '\0' && strcmp(value, "0") != 0;
    }();
    auto reportStage = [&path](const char *stage, const std::string &before, const std::string &after) {
        if (reportShims && before != after) {
            fprintf(stderr, "[fx9] source shim applied: %s (%s)\n", stage, path.c_str());
        }
    };
    std::string stage = runCompatibilityRules(path, source);
    reportStage("compatibility", source, stage);
    std::string normalized = runSourceNormalizer(stage);
    reportStage("normalizer", stage, normalized);
    std::string expanded = runLegacyEffectRules(normalized);
    reportStage("legacy", normalized, expanded);
    std::string hygiened = normalizeRedefineMacros(expanded);
    reportStage("macro-hygiene", expanded, hygiened);
    return hygiened;
}

} /* namespace translation */
} /* namespace fx9 */

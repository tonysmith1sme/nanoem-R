/*
   Copyright (c) 2015-2023 hkrn All rights reserved

   This file is licensed under MIT license. for more details, see LICENSE.txt.
 */

#include "fx9/EffectSourcePatch.h"

#include <cctype>
#include <cstdio>
#include <cstring>
#include <regex>
#include <sstream>
#include <string>
#include <unordered_map>

namespace fx9 {
namespace effect {
namespace {

std::string
toLowerASCII(const std::string &value)
{
    std::string normalized(value);
    for (size_t i = 0, numChars = normalized.size(); i < numChars; i++) {
        normalized[i] = static_cast<char>(tolower(static_cast<unsigned char>(normalized[i])));
    }
    return normalized;
}

bool
containsRayMMDPath(const std::string &path)
{
    return toLowerASCII(path).find("ray-mmd") != std::string::npos;
}

bool
endsWithIgnoreCase(const std::string &value, const char *suffix)
{
    const std::string valueLC(toLowerASCII(value)), suffixLC(toLowerASCII(std::string(suffix)));
    return valueLC.size() >= suffixLC.size() &&
        valueLC.compare(valueLC.size() - suffixLC.size(), suffixLC.size(), suffixLC) == 0;
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

bool
isRayMMDMainEffectSourcePath(const std::string &path)
{
    std::string normalized(toLowerASCII(path));
    for (size_t i = 0, numChars = normalized.size(); i < numChars; i++) {
        if (normalized[i] == '\\') {
            normalized[i] = '/';
        }
    }
    return normalized.find("ray-mmd") != std::string::npos && normalized.find("/main/main.fxsub") != std::string::npos;
}

bool
isRayMMDShaderSourcePath(const std::string &path)
{
    std::string normalized(toLowerASCII(path));
    for (size_t i = 0, numChars = normalized.size(); i < numChars; i++) {
        if (normalized[i] == '\\') {
            normalized[i] = '/';
        }
    }
    return normalized.find("ray-mmd") != std::string::npos && normalized.find("/shader/") != std::string::npos;
}

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
patchRayMMDMetalHeavySamplingSource(const std::string &path, const std::string &source)
{
    if (!isRayMMDShaderSourcePath(path)) {
        return source;
    }
    std::string patched(source);
    patched = std::regex_replace(patched, std::regex(R"(\[\s*unroll\s*\])", std::regex_constants::icase), "[loop]");
    patched = std::regex_replace(patched, std::regex(R"(#\s*define\s+SSDO_UNROLL\s+\[\s*unroll\s*\])",
                                          std::regex_constants::icase),
        "#define SSDO_UNROLL [loop]");
    patched = std::regex_replace(patched, std::regex(R"(#\s*define\s+SSDO_UNROLL\s*(?:\r?\n))",
                                          std::regex_constants::icase),
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
patchRayMMDConfigurationInclude(const std::string &source)
{
    static const char *kRayMMDMetalConfigurationOverride =
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
    return std::regex_replace(source,
        std::regex(R"((#\s*include\s+["<]ray\.conf[">]\s*))", std::regex_constants::icase),
        std::string("$1") + kRayMMDMetalConfigurationOverride);
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

bool
isValidHlslStringEscapeFollower(char c)
{
    /* HLSL string literal characters allowed immediately after a single backslash. */
    return c == '\\' || c == '"' || c == '\'' || c == '?' || c == 'a' || c == 'b' || c == 'f' ||
        c == 'n' || c == 'r' || c == 't' || c == 'v' || c == 'x' || c == 'u' ||
        (c >= '0' && c <= '7');
}

std::string
patchStrayBackslashesInStringLiterals(const std::string &source)
{
    /* Some legacy MME effects ship as Shift-JIS source where multi-byte characters contain the
       byte 0x5c (= ASCII backslash) as their second byte. After Windows code-page-932 to UTF-8
       conversion a literal `\` may end up between two non-ASCII characters inside a HLSL string
       literal (e.g. HgSAO.fx "AO表\示"), which the HLSL lexer rejects as "Invalid escape sequence".
       Walk each string literal and duplicate any `\` that is not followed by a valid escape leader
       so the lexer reads it as a literal backslash instead. */
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
                /* unterminated string: bail out and copy the remainder verbatim */
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

} /* namespace anonymous */

std::string
patchRayMMDSource(const std::string &path, const std::string &source)
{
    if (!containsRayMMDPath(path)) {
        return source;
    }
    std::string patched(source);
    if (endsWithIgnoreCase(path, "ray.conf")) {
        const std::regex_constants::syntax_option_type flags = std::regex_constants::ECMAScript |
            std::regex_constants::icase;
        patched = replaceRayMMDIntegerDefine(patched, "OUTLINE_QUALITY", 1, flags);
    }
    if (endsWithIgnoreCase(path, "PostProcessDiffusion.fxsub")) {
        patched = std::string(
            "#if !defined(SSDO_QUALITY) || SSDO_QUALITY == 0\n"
            "float linearizeDepth(float2 uv) { return tex2Dlod(Gbuffer8Map, float4(uv, 0, 0)).r; }\n"
            "#endif\n") + patched;
    }
    if (endsWithIgnoreCase(path, "ShadingMaterials.fxsub")) {
        for (size_t pos = 0; (pos = patched.find("diffuse += tex2Dlod(LightMapSamp, float4(coord, 0, 0)).rgb;", pos)) != std::string::npos; pos += 62) {
            patched.replace(pos, 62, "diffuse += max(tex2Dlod(LightMapSamp, float4(coord, 0, 0)).rgb, 0.0);");
        }
        for (size_t pos = 0; (pos = patched.find("specular += tex2Dlod(LightSpecMapSamp, float4(coord, 0, 0)).rgb;", pos)) != std::string::npos; pos += 67) {
            patched.replace(pos, 67, "specular += max(tex2Dlod(LightSpecMapSamp, float4(coord, 0, 0)).rgb, 0.0);");
        }
        patched = std::regex_replace(patched,
            std::regex(R"(diffuse \+= iblDiffuse;)"),
            "diffuse += max(0.0, min(iblDiffuse, 1e10));");
        patched = std::regex_replace(patched,
            std::regex(R"(specular \+= iblSpecular;)"),
            "specular += max(0.0, min(iblSpecular, 1e10));");
        patched = std::regex_replace(patched,
            std::regex("(oColor0 = float4\\(diffuse \\* material\\.albedo \\+ specular, material\\.linearDepth\\);)"),
            "diffuse = max(diffuse, max(material.albedo, 0.02) * 0.08);\n\t"
            "diffuse += material.albedo * 0.04;\n\t"
            "specular = max(specular, float3(0.005, 0.005, 0.005));\n\t$1");
        // Fix: Bypass DecodeYcbcr in ShadingImageBasedLighting on Metal.
        // Even with YCbCr encoding bypassed in the skybox writers, MSL may still
        // produce channel-misaligned reads. Read packed .rgb/.rgb directly instead.
        patched = std::regex_replace(patched,
            std::regex(R"(DecodeYcbcr\(source\s*,\s*coord\s*,\s*screenPosition\s*,\s*ViewportOffset2\s*,\s*diffuse\s*,\s*specular\s*\)\s*;)"),
            "{ float4 __ibl = tex2Dlod(source, float4(coord, 0, 0)); "
            "float3 __iblRgb = clamp(__ibl.rgb, 0.0, 8.0); "
            "if (any(isnan(__ibl.rgb)) || any(isinf(__ibl.rgb))) __iblRgb = 0.0; "
            "diffuse = __iblRgb; specular = __iblRgb; }");
    }
    if (endsWithIgnoreCase(path, "Sky with lighting.fx")) {
        patched = std::regex_replace(patched,
            std::regex("(#include\\s+\"[^\"]+\"\\s*)"),
            "#define MIDPOINT_8_BIT (127.0f / 255.0f)\n$1",
            std::regex_constants::format_first_only);
        // Fix: Skip YCbCr encoding on Metal to avoid purple overlay.
        // EncodeYcbcr/DecodeYcbcr uses fmod + EdgeFilter 4-neighbor sampling,
        // which produces channel misalignment under MSL's float ordering.
        patched = std::regex_replace(patched,
            std::regex(R"(oColor0\s*=\s*EncodeYcbcr\s*\(\s*screenPosition\s*,\s*diffuse\s*,\s*specular\s*\)\s*;)"),
            "oColor0 = float4(diffuse, 0);");
        patched = std::regex_replace(patched,
            std::regex(R"(oColor1\s*=\s*EncodeYcbcr\s*\(\s*screenPosition\s*,\s*diffuse2\s*,\s*specular2\s*\)\s*;)"),
            "oColor1 = float4(specular, 0);");
    }
    if (endsWithIgnoreCase(path, "skylighting.fxsub") ||
        endsWithIgnoreCase(path, "skylighting_fast.fxsub")) {
        // Fix: Same YCbCr bypass as Sky with lighting.fx — these are the actual
        // writers of EnvLightMap when Sky Hemisphere/Sky with lighting is used,
        // and they suffer from the same MSL purple-overlay channel misalignment.
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
            "void MaterialPS(in float3 normal : TEXCOORD0, in float2 coord : TEXCOORD1, in float4 worldPos : TEXCOORD2, in float4 viewdir : TEXCOORD3, out float4 oColor0 : COLOR0, out float4 oColor1 : COLOR1, out float4 oColor2 : COLOR2, out float4 oColor3 : COLOR3)");
        patched = std::regex_replace(patched,
            std::regex("GbufferParam MaterialPS\\("
                       "\\s*in float3 normal\\s*:\\s*TEXCOORD0\\s*,"
                       "\\s*in float2 coord\\s*:\\s*TEXCOORD1\\s*,"
                       "\\s*in float4 worldPos\\s*:\\s*TEXCOORD2\\s*\\)"),
            "void MaterialPS(in float3 normal : TEXCOORD0, in float2 coord : TEXCOORD1, in float4 worldPos : TEXCOORD2, out float4 oColor0 : COLOR0, out float4 oColor1 : COLOR1, out float4 oColor2 : COLOR2, out float4 oColor3 : COLOR3)");
        patched = std::regex_replace(patched,
            std::regex("GbufferParam Material2PS\\("
                       "\\s*in float3 normal\\s*:\\s*TEXCOORD0\\s*,"
                       "\\s*in float2 coord\\s*:\\s*TEXCOORD1\\s*,"
                       "\\s*in float4 worldPos\\s*:\\s*TEXCOORD2\\s*\\)"),
            "void Material2PS(in float3 normal : TEXCOORD0, in float2 coord : TEXCOORD1, in float4 worldPos : TEXCOORD2, out float4 oColor0 : COLOR0, out float4 oColor1 : COLOR1, out float4 oColor2 : COLOR2, out float4 oColor3 : COLOR3)");
        patched = std::regex_replace(patched,
            std::regex("return EncodeGbuffer\\(material, (\\w+)\\.w\\);"),
            "GbufferParam __gb = EncodeGbuffer(material, $1.w); "
            "oColor0 = __gb.buffer1; oColor1 = __gb.buffer2; "
            "oColor2 = __gb.buffer3; oColor3 = __gb.buffer4;");
    }
    if (endsWithIgnoreCase(path, "_2.0.fxsub")) {
        replaceAll(patched, "\r\n", "\n");
        replaceAll(patched,
            "GbufferParam MaterialPS(\n\tin float3 normal   : TEXCOORD0,\n\tin float2 coord0   : TEXCOORD1,\n\tin float4 worldPos : TEXCOORD3)",
            "void MaterialPS(\n\tin float3 normal   : TEXCOORD0,\n\tin float2 coord0   : TEXCOORD1,\n\tin float4 worldPos : TEXCOORD3, out float4 oColor0 : COLOR0, out float4 oColor1 : COLOR1, out float4 oColor2 : COLOR2, out float4 oColor3 : COLOR3)");
        replaceAll(patched,
            "GbufferParam MaterialPS(\n\tin float3 normal   : TEXCOORD0,\n\tin float2 coord0   : TEXCOORD1,\n#if OCCLUSION_MAP_TYPE == 2 || OCCLUSION_MAP_TYPE == 3\n\tin float2 coord1   : TEXCOORD2,\n#endif\n\tin float4 worldPos : TEXCOORD3)",
            "void MaterialPS(\n\tin float3 normal   : TEXCOORD0,\n\tin float2 coord0   : TEXCOORD1,\n#if OCCLUSION_MAP_TYPE == 2 || OCCLUSION_MAP_TYPE == 3\n\tin float2 coord1   : TEXCOORD2,\n#endif\n\tin float4 worldPos : TEXCOORD3, out float4 oColor0 : COLOR0, out float4 oColor1 : COLOR1, out float4 oColor2 : COLOR2, out float4 oColor3 : COLOR3)");
        replaceAll(patched,
            "GbufferParam Material2PS(\n\tin float3 normal   : TEXCOORD0,\n\tin float2 coord0   : TEXCOORD1,\n\tin float4 worldPos : TEXCOORD3)",
            "void Material2PS(\n\tin float3 normal   : TEXCOORD0,\n\tin float2 coord0   : TEXCOORD1,\n\tin float4 worldPos : TEXCOORD3, out float4 oColor0 : COLOR0, out float4 oColor1 : COLOR1, out float4 oColor2 : COLOR2, out float4 oColor3 : COLOR3)");
        replaceAll(patched,
            "GbufferParam Material2PS(\n\tin float3 normal   : TEXCOORD0,\n\tin float2 coord0   : TEXCOORD1,\n#if OCCLUSION_MAP_TYPE == 2 || OCCLUSION_MAP_TYPE == 3\n\tin float2 coord1   : TEXCOORD2,\n#endif\n\tin float4 worldPos : TEXCOORD3)",
            "void Material2PS(\n\tin float3 normal   : TEXCOORD0,\n\tin float2 coord0   : TEXCOORD1,\n#if OCCLUSION_MAP_TYPE == 2 || OCCLUSION_MAP_TYPE == 3\n\tin float2 coord1   : TEXCOORD2,\n#endif\n\tin float4 worldPos : TEXCOORD3, out float4 oColor0 : COLOR0, out float4 oColor1 : COLOR1, out float4 oColor2 : COLOR2, out float4 oColor3 : COLOR3)");
        replaceAll(patched,
            "\treturn EncodeGbuffer(material, worldPos.w);",
            "\tGbufferParam __gb = EncodeGbuffer(material, worldPos.w);\n\toColor0 = __gb.buffer1; oColor1 = __gb.buffer2; oColor2 = __gb.buffer3; oColor3 = __gb.buffer4;");
    }
    if (endsWithIgnoreCase(path, "PostProcessHDR.fxsub") ||
        endsWithIgnoreCase(path, "LightBloom.fxsub")) {
        patched = std::regex_replace(patched, std::regex(R"(\bsampler\b\s+(\w+)\s*,)"), "sampler2D $1,");
        patched = std::regex_replace(patched, std::regex(R"(\bsampler\b\s+(\w+)\s*\))"), "sampler2D $1)");
    }
    if (endsWithIgnoreCase(path, "ShadowMap.fxsub")) {
        replaceAll(patched, "SHADOW_BLUR_COUNT 6", "SHADOW_BLUR_COUNT 24");
        replaceAll(patched, "float radius = 2.0 / SHADOW_MAP_SIZE", "float radius = 48.0 / SHADOW_MAP_SIZE");
        replaceAll(patched, "exp(-20 *", "exp(-2 *");
        replaceAll(patched, "float2 offset1 = coord + offset", "float2 offset1 = coord + offset * 8");
        replaceAll(patched, "float2 offset2 = coord - offset", "float2 offset2 = coord - offset * 8");
        // Fix: Use fixed sigma=8 for BilateralWeight to maintain proper depth-aware filtering
        // SHADOW_BLUR_COUNT (24) is too large as sigma - makes bilateral filter nearly uniform
        replaceAll(patched,
            "BilateralWeight(r, depth1, center_d, SHADOW_BLUR_COUNT, 10)",
            "BilateralWeight(r, depth1, center_d, 10, 10)");
        replaceAll(patched,
            "BilateralWeight(r, depth2, center_d, SHADOW_BLUR_COUNT, 10)",
            "BilateralWeight(r, depth2, center_d, 10, 10)");
    }
    if (endsWithIgnoreCase(path, "textures.fxsub")) {
        // Keep L8 format on Metal to avoid R16F sampling issues
        // replaceAll(patched,
        //     "texture ShadowMap : RENDERCOLORTARGET<\r\n\tfloat2 ViewportRatio = {1.0, 1.0};\r\n\tstring Format = \"L8\";\r\n>;",
        //     "texture ShadowMap : RENDERCOLORTARGET<\r\n\tfloat2 ViewportRatio = {1.0, 1.0};\r\n\tstring Format = \"R16F\";\r\n>;");
        // replaceAll(patched,
        //     "texture ShadowMapTemp : RENDERCOLORTARGET<\r\n\tfloat2 ViewportRatio = {1.0, 1.0};\r\n\tstring Format = \"L8\";\r\n>;",
        //     "texture ShadowMapTemp : RENDERCOLORTARGET<\r\n\tfloat2 ViewportRatio = {1.0, 1.0};\r\n\tstring Format = \"R16F\";\r\n>;");
    }
    if (endsWithIgnoreCase(path, "FXAA3.fxsub")) {
        patched = std::regex_replace(patched,
            std::regex("\\}(FxaaFloat4 FxaaPixelShader)"),
            "}\n$1");
        patched = std::regex_replace(patched,
            std::regex("(\\{)(FxaaFloat2 posM)"),
            "{\n$2");
        patched = std::regex_replace(patched,
            std::regex(";(FxaaFloat lumaM)"),
            ";\n$1");
        patched = std::regex_replace(patched,
            std::regex(";(FxaaBool earlyExit)"),
            ";\n$1");
        patched = std::regex_replace(patched,
            std::regex(";(FxaaFloat lumaNW)"),
            ";\n$1");
    }
    patched = patchRayMMDConfigurationInclude(patched);
    patched = std::regex_replace(patched, std::regex("Sky\\*box\\*\\.\\*", std::regex_constants::icase),
        "sky*box*.*");
    patched = std::regex_replace(patched,
        std::regex(R"(float2\s+SHKernel\s*\[\s*6\s*\]\s*\[\s*9\s*\])", std::regex_constants::icase),
        "float2 SHKernel[54]");
    patched = std::regex_replace(patched,
        std::regex(R"(SHKernel\s*\[\s*([^\]]+)\s*\]\s*\[\s*([^\]]+)\s*\])", std::regex_constants::icase),
        "SHKernel[(($1) * 9) + ($2)]");
    if (isRayMMDMainEffectSourcePath(path)) {
        patched = std::regex_replace(patched,
            std::regex(R"(return\s+float4\s*\(\s*GetSpecularHighlight\s*\(\s*normal\s*,\s*coord\s*\)\s*,\s*0\s*\)\s*;)"),
            "return float4(MaterialDiffuse.rgb * alpha, alpha);");
    }
    patched = patchRayMMDMetalHeavySamplingSource(path, patched);
    return patched;
}


std::string
patchLegacyEffectSource(const std::string &path, const std::string &source)
{
    std::string patched(patchRayMMDSource(path, source)), result;
    {
        std::string fixed;
        fixed.reserve(patched.size());
        for (size_t i = 0; i < patched.size(); i++) {
            if (static_cast<unsigned char>(patched[i]) == 0xC2 &&
                i + 1 < patched.size() && static_cast<unsigned char>(patched[i + 1]) == 0xA5) {
                fixed.push_back('\\');
                i++;
            } else {
                fixed.push_back(patched[i]);
            }
        }
        patched = std::move(fixed);
    }
    patched = patchStrayBackslashesInStringLiterals(patched);
    if (patched.find("DefTech(") != std::string::npos &&
        patched.find("#define DefTech(") != std::string::npos) {
        bool isNormalDraw = patched.find("Subset=EdgeMaterial") != std::string::npos;
        size_t macroPos = patched.find("#define DefTech(");
        size_t scanPos = patched.find('\n', macroPos);
        if (scanPos != std::string::npos) {
            scanPos++;
            while (scanPos < patched.size()) {
                size_t nl = patched.find('\n', scanPos);
                if (nl == std::string::npos) break;
                size_t endChar = nl;
                while (endChar > scanPos && (patched[endChar - 1] == '\r' || patched[endChar - 1] == ' ' || patched[endChar - 1] == '\t')) {
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
        const char *defTechArgs[] = {
            "_0, object , true, Tex)",
            "_1, object , false, NoTex)",
            "_2, object_ss , true, Tex)",
            "_3, object_ss , false, NoTex)"
        };
        for (size_t pos = 0; (pos = patched.find("DefTech(", pos)) != std::string::npos; ) {
            size_t end = patched.find('\n', pos);
            if (end == std::string::npos) end = patched.size();
            std::string line = patched.substr(pos, end - pos);
            bool matched = false;
            for (size_t i = 0; i < 4 && !matched; i++) {
                if (line.find(std::string("DefTech(") + defTechArgs[i]) == 0) {
                    const char *passName = (i >= 2) ? "object_ss" : "object";
                    const char *useTex = (i % 2 == 0) ? "true" : "false";
                    const char *texFunc = (i % 2 == 0) ? "Object_Tex_PS" : "Object_NoTex_PS";
                    if (isNormalDraw) {
                        char buf[1024];
                        snprintf(buf, sizeof(buf),
                            "technique ObjectEdgeTec_%zu < string MMDPass = \"%s\"; string Subset=EdgeMaterial; bool UseTexture=%s;> { pass DrawEdge { AlphaBlendEnable = FALSE; AlphaTestEnable = FALSE; VertexShader = compile vs_2_0 Object_VS(); PixelShader = compile ps_2_0 %s(1); } } technique ObjectNoEdgeTec_%zu < string MMDPass = \"%s\"; bool UseTexture=%s;> { pass DrawEdge { AlphaBlendEnable = FALSE; AlphaTestEnable = FALSE; VertexShader = compile vs_2_0 Object_VS(); PixelShader = compile ps_2_0 %s(0); } }",
                            i, passName, useTex, texFunc, i, passName, useTex, texFunc);
                        patched.replace(pos, line.size(), buf);
                        pos += strlen(buf);
                    }
                    else {
                        char buf[1024];
                        snprintf(buf, sizeof(buf),
                            "technique ObjectTec_%zu < string MMDPass = \"%s\"; bool UseTexture=%s;> { pass DrawEdge { AlphaBlendEnable = true; VertexShader = compile vs_2_0 Object_VS(); PixelShader = compile ps_2_0 %s(); } }",
                            i, passName, useTex, texFunc);
                        patched.replace(pos, line.size(), buf);
                        pos += strlen(buf);
                    }
                    matched = true;
                }
            }
            if (!matched) {
                pos = end + 1;
            }
        }
    }
    std::unordered_map<std::string, std::string> macroSubstitutions;
    size_t offset = 0;
    while (offset <= patched.size()) {
        const size_t end = patched.find('\n', offset);
        const bool hasLineFeed = end != std::string::npos;
        const std::string line(hasLineFeed ? patched.substr(offset, end - offset) : patched.substr(offset));
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

} /* namespace effect */
} /* namespace fx9 */

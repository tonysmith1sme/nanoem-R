/*
   Copyright (c) 2015-2023 hkrn All rights reserved

   This file is licensed under MIT license. for more details, see LICENSE.txt.
 */

#define CATCH_CONFIG_FAST_COMPILE
#include <catch2/catch.hpp>

#include "fx9next/Compiler.h"
#include "fx9next/Parser.h"

#include "effect.pb-c.h"

#include <cstdio>
#include <cstdlib>
#include <string>
#include <vector>

#if defined(__APPLE__)
#include <unistd.h>
#endif

using namespace fx9next;

bool
compileMetalSource(const char *source, const char *label)
{
#if defined(__APPLE__)
    if (std::system("xcrun --find metal >/dev/null 2>&1") != 0) {
        WARN("Apple Metal compiler is unavailable");
        return true;
    }
    char input[] = "/tmp/fx9next-metal-XXXXXX";
    const int fd = mkstemp(input);
    if (fd < 0) {
        WARN("cannot create temporary Metal source");
        return false;
    }
    FILE *fp = fdopen(fd, "w");
    if (!fp) {
        close(fd);
        std::remove(input);
        return false;
    }
    std::fputs(source, fp);
    std::fclose(fp);
    std::string output(input);
    output += ".air";
    const std::string command = "xcrun metal -c " + output.substr(0, output.size() - 4) + " -o " + output +
        " >/dev/null 2>&1";
    const bool ok = std::system(command.c_str()) == 0;
    if (!ok) {
        WARN(std::string("Apple Metal compiler rejected ") + label);
    }
    std::remove(input);
    std::remove(output.c_str());
    return ok;
#else
    (void) source;
    (void) label;
    return true;
#endif
}

TEST_CASE("fx9next parses a pass-through effect")
{
    const char *src =
        "float4 vs_main(float4 position : POSITION) : POSITION { return position; }\n"
        "float4 ps_main() : COLOR0 { return float4(1, 1, 1, 1); }\n"
        "technique t { pass p {\n"
        "  VertexShader = compile vs_3_0 vs_main();\n"
        "  PixelShader = compile ps_3_0 ps_main();\n"
        "} }\n";
    TranslationUnit unit;
    std::string err;
    Parser parser;
    REQUIRE(parser.parse(src, "pass.fx", unit, err));
    REQUIRE(err.empty());
    REQUIRE(unit.functions.size() == 2);
    REQUIRE(unit.techniques.size() == 1);
    REQUIRE(unit.techniques[0].passes.size() == 1);
    REQUIRE(unit.techniques[0].passes[0].vsEntry == "vs_main");
    REQUIRE(unit.techniques[0].passes[0].psEntry == "ps_main");
}

TEST_CASE("fx9next preserves sampler texture relationship")
{
    const char *src =
        "texture2D diffuseTexture : DIFFUSE;\n"
        "sampler2D diffuseSampler = sampler_state { Texture = <diffuseTexture>; };\n"
        "float4 vs_main(float4 position : POSITION) : POSITION { return position; }\n"
        "float4 ps_main(float2 uv : TEXCOORD0) : COLOR0 { return tex2D(diffuseSampler, uv); }\n"
        "technique t { pass p { VertexShader = compile vs_3_0 vs_main(); "
        "PixelShader = compile ps_3_0 ps_main(); } }\n";
    Compiler compiler;
    compiler.setTargetLanguage(Compiler::kLanguageTypeGLSL);
    Compiler::EffectProduct product;
    REQUIRE(compiler.compile(std::string(src), "sampler.fx", product));
    Fx9__Effect__Effect *effect =
        fx9__effect__effect__unpack(nullptr, product.message.size(), product.message.data());
    REQUIRE(effect != nullptr);
    REQUIRE(effect->techniques[0]->passes[0]->pixel_shader->n_samplers == 1);
    REQUIRE(std::string(effect->techniques[0]->passes[0]->pixel_shader->samplers[0]->texture_name) ==
        "diffuseTexture");
    fx9__effect__effect__free_unpacked(effect, nullptr);
}

TEST_CASE("fx9next exposes vertex sampler metadata")
{
    const char *src =
        "texture2D diffuseTexture : DIFFUSE;\n"
        "sampler2D diffuseSampler = sampler_state { Texture = <diffuseTexture>; };\n"
        "float4 vs_main(float4 position : POSITION) : POSITION { return position + tex2D(diffuseSampler, float2(0, 0)); }\n"
        "float4 ps_main() : COLOR0 { return float4(1, 1, 1, 1); }\n"
        "technique t { pass p { VertexShader = compile vs_3_0 vs_main(); "
        "PixelShader = compile ps_3_0 ps_main(); } }\n";
    Compiler compiler;
    compiler.setTargetLanguage(Compiler::kLanguageTypeMSL);
    Compiler::EffectProduct product;
    REQUIRE(compiler.compile(std::string(src), "vertex-sampler.fx", product));
    Fx9__Effect__Effect *effect =
        fx9__effect__effect__unpack(nullptr, product.message.size(), product.message.data());
    REQUIRE(effect != nullptr);
    Fx9__Effect__Pass *pass = effect->techniques[0]->passes[0];
    REQUIRE(pass->vertex_shader->n_samplers == 1);
    REQUIRE(std::string(pass->vertex_shader->samplers[0]->texture_name) == "diffuseTexture");
    REQUIRE(pass->vertex_shader->msl != nullptr);
    REQUIRE(compileMetalSource(pass->vertex_shader->msl, "vertex texture shader"));
    fx9__effect__effect__free_unpacked(effect, nullptr);
}

TEST_CASE("fx9next accepts D3D11 texture and sampler declarations")
{
    const char *src =
        "Texture2D diffuseTexture : register(t3);\n"
        "SamplerState diffuseSampler : register(s2);\n"
        "float4 vs_main(float4 position : POSITION) : POSITION { return position; }\n"
        "float4 ps_main(float2 uv : TEXCOORD0) : COLOR0 { return tex2D(diffuseSampler, uv); }\n"
        "technique t { pass p { VertexShader = compile vs_4_0 vs_main(); "
        "PixelShader = compile ps_4_0 ps_main(); } }\n";
    Compiler compiler;
    compiler.setTargetLanguage(Compiler::kLanguageTypeMSL);
    Compiler::EffectProduct product;
    REQUIRE(compiler.compile(std::string(src), "d3d11-declarations.fx", product));
    Fx9__Effect__Effect *effect =
        fx9__effect__effect__unpack(nullptr, product.message.size(), product.message.data());
    REQUIRE(effect != nullptr);
    Fx9__Effect__Pass *pass = effect->techniques[0]->passes[0];
    REQUIRE(pass->pixel_shader->n_samplers == 1);
    REQUIRE(pass->pixel_shader->samplers[0]->index == 2);
    REQUIRE(pass->pixel_shader->msl != nullptr);
    REQUIRE(compileMetalSource(pass->pixel_shader->msl, "D3D11 sampler shader"));
    fx9__effect__effect__free_unpacked(effect, nullptr);
}

TEST_CASE("fx9next lowers D3D11 constant buffer fields")
{
    const char *src =
        "cbuffer ConstantBufferData : register(b0) { float4 tint; float exposure; };\n"
        "float4 vs_main(float4 position : POSITION) : POSITION { return position; }\n"
        "float4 ps_main() : COLOR0 { return tint * exposure; }\n"
        "technique t { pass p { VertexShader = compile vs_5_0 vs_main(); "
        "PixelShader = compile ps_5_0 ps_main(); } }\n";
    Compiler compiler;
    compiler.setTargetLanguage(Compiler::kLanguageTypeMSL);
    Compiler::EffectProduct product;
    REQUIRE(compiler.compile(std::string(src), "d3d11-cbuffer.fx", product));
    Fx9__Effect__Effect *effect =
        fx9__effect__effect__unpack(nullptr, product.message.size(), product.message.data());
    REQUIRE(effect != nullptr);
    Fx9__Effect__Pass *pass = effect->techniques[0]->passes[0];
    REQUIRE(pass->pixel_shader->n_uniforms == 2);
    REQUIRE(pass->pixel_shader->uniforms[0]->index == 0);
    REQUIRE(pass->pixel_shader->uniforms[1]->index == 1);
    REQUIRE(pass->pixel_shader->msl != nullptr);
    REQUIRE(compileMetalSource(pass->pixel_shader->msl, "D3D11 constant buffer shader"));
    fx9__effect__effect__free_unpacked(effect, nullptr);
}

TEST_CASE("fx9next compiles pass-through effect to protobuf")
{
    const char *src =
        "float4 vs_main(float4 position : POSITION) : POSITION { return position; }\n"
        "float4 ps_main() : COLOR0 { return float4(1, 1, 1, 1); }\n"
        "technique t { pass p {\n"
        "  VertexShader = compile vs_3_0 vs_main();\n"
        "  PixelShader = compile ps_3_0 ps_main();\n"
        "} }\n";
    Compiler compiler;
    compiler.setTargetLanguage(Compiler::kLanguageTypeGLSL);
    Compiler::EffectProduct product;
    const bool ok = compiler.compile(std::string(src), "pass.fx", product);
    INFO(product.sink.info);
    if (!product.sink.translator.empty()) {
        for (auto it = product.sink.translator.begin(); it != product.sink.translator.end(); ++it) {
            INFO(*it);
        }
    }
    INFO(product.sink.builder);
    REQUIRE(ok);
    REQUIRE(product.numPasses == 1);
    REQUIRE(product.numCompiledPasses == 1);
    REQUIRE_FALSE(product.message.empty());
}

TEST_CASE("fx9next compiles fx9 corpus effects")
{
    static const char *kFiles[] = { "01_tex2D_basic.fx", "02_tex2Dlod_bias_proj.fx", "03_texM3x3.fx",
        "04_token_pasting.fx", "05_many_samplers.fx", "06_alpha_test_states.fx", "07_stencil_states.fx",
        "08_two_sided_stencil.fx", "09_script_commands.fx", "10_dummy_pass.fx", "11_shift_jis.fx",
        "12_string_annotation.fx", "14_matrix_semantics.fx", "15_vpos_vface.fx", "16_matrix_swizzle.fx",
        "17_modulo_fmod.fx", "18_ternary_bool.fx", "19_struct_io.fx", "20_matrix_cast.fx" };
    const std::string name = GENERATE(from_range(std::vector<std::string>(kFiles, kFiles + 19)));
    const std::string path = std::string(FX9NEXT_TEST_EFFECT_FIXTURES_PATH) + "/corpus/" + name;
    Compiler compiler;
    compiler.setTargetLanguage(Compiler::kLanguageTypeGLSL);
    Compiler::EffectProduct product;
    const bool ok = compiler.compile(path.c_str(), product);
    INFO(path);
    INFO(product.sink.info);
    INFO(product.sink.builder);
    if (!product.sink.translator.empty()) {
        for (auto it = product.sink.translator.begin(); it != product.sink.translator.end(); ++it) {
            INFO(*it);
        }
    }
    REQUIRE(ok);
    REQUIRE(product.numCompiledPasses == product.numPasses);
    REQUIRE_FALSE(product.message.empty());
}

TEST_CASE("fx9next compiles MME filter packs")
{
    static const char *kFiles[] = { "Diffusion7/Diffusion.fx", "AnimeScreenTex_v1.0/A-screen.fx",
        "LightBloom/LightBloom with DirtMap.fx", "msUnsharp/msUnsharp.fx", "ikBokeh_v020a_SJ/ikBokeh.fx",
        "ikDiffusion/ikDiffusion/Diffusion1/ikDiffusion1.fx", "ikDiffusion/ikDiffusion/Diffusion2/ikDiffusion2.fx",
        "PostAdultShaderS2_v013/PostAdultShader.fx" };
    const std::string name = GENERATE(from_range(std::vector<std::string>(kFiles, kFiles + 8)));
    const std::string path = std::string(FX9NEXT_TEST_EFFECT_FIXTURES_PATH) + "/../../../../MME/" + name;
    FILE *fp = std::fopen(path.c_str(), "rb");
    if (!fp) {
        WARN("missing " + path);
        return;
    }
    std::fclose(fp);
    Compiler compiler;
    compiler.setTargetLanguage(Compiler::kLanguageTypeGLSL);
    compiler.setDefineMacro("NANOEM", "1");
    Compiler::EffectProduct product;
    const bool ok = compiler.compile(path.c_str(), product);
    INFO(path);
    INFO(product.sink.info);
    INFO(product.sink.builder);
    if (!product.sink.translator.empty()) {
        for (auto it = product.sink.translator.begin(); it != product.sink.translator.end(); ++it) {
            INFO(*it);
        }
    }
    REQUIRE(ok);
}

TEST_CASE("fx9next ray-mmd compile progress")
{
    static const char *kFiles[] = { "ray-mmd-1.5.2/Main/main.fx", "ray-mmd-1.5.2/ray.fx",
        "ray-mmd-1.5.2/Extension/FXAA/FXAA.fx", "ray-mmd-1.5.2/Materials/Toon/Toon.fx",
        "ray-mmd-1.5.2/Materials/Transparent/material_glass.fx",
        "ray-mmd-1.5.2/Materials/Hair/material_hair.fx",
        "ray-mmd-1.5.2/Materials/Subsurface/material_jade_white.fx",
        "ray-mmd-1.5.2/Materials/Video/material_screen.fx",
        "ray-mmd-1.5.2/Fog/AtmosphericFog/atmospheric_fog.fx",
        "ray-mmd-1.5.2/Fog/GroundFog/ground_fog.fx",
        "ray-mmd-1.5.2/Shadow/PSSM1.fx",
        "ray-mmd-1.5.2/Skybox/Sky Night/Sky with box.fx",
        "ray-mmd-1.5.2/Extension/ColorGrading/ColorGrading.fx" };
    const std::string name = GENERATE(from_range(std::vector<std::string>(kFiles, kFiles + 13)));
    const std::string path = std::string(FX9NEXT_TEST_EFFECT_FIXTURES_PATH) + "/../../../../MME/" + name;
    FILE *fp = std::fopen(path.c_str(), "rb");
    if (!fp) {
        WARN("missing " + path);
        return;
    }
    std::fclose(fp);
    Compiler compiler;
    compiler.setTargetLanguage(Compiler::kLanguageTypeGLSL);
    compiler.setDefineMacro("NANOEM", "1");
    compiler.setDefineMacro("FOG_ENABLE", "0");
    Compiler::EffectProduct product;
    const bool ok = compiler.compile(path.c_str(), product);
    INFO(path);
    INFO(product.sink.info);
    INFO(product.sink.builder);
    if (!product.sink.translator.empty()) {
        for (auto it = product.sink.translator.begin(); it != product.sink.translator.end(); ++it) {
            INFO(*it);
        }
    }
    CHECK(ok);
    CHECK(product.numPasses > 0);
    CHECK(product.numCompiledPasses == product.numPasses);
    CHECK_FALSE(product.message.empty());
}

TEST_CASE("fx9next compiles corpus to hlsl and msl")
{
    const char *src =
        "float4 vs_main(float4 position : POSITION) : POSITION { return position; }\n"
        "float4 ps_main() : COLOR0 { return float4(1, 1, 1, 1); }\n"
        "technique t { pass p {\n"
        "  VertexShader = compile vs_3_0 vs_main();\n"
        "  PixelShader = compile ps_3_0 ps_main();\n"
        "} }\n";
    const LanguageType language = GENERATE(kLanguageTypeHLSL, kLanguageTypeMSL);
    Compiler compiler;
    compiler.setTargetLanguage(language);
    Compiler::EffectProduct product;
    const bool ok = compiler.compile(std::string(src), "pass.fx", product);
    INFO(product.sink.info);
    INFO(product.sink.builder);
    REQUIRE(ok);
    REQUIRE_FALSE(product.message.empty());
}

TEST_CASE("fx9next validates a pass-through product")
{
    const char *src =
        "float4 vs_main(float4 position : POSITION) : POSITION { return position; }\n"
        "float4 ps_main() : COLOR0 { return float4(1, 1, 1, 1); }\n"
        "technique t { pass p {\n"
        "  VertexShader = compile vs_3_0 vs_main();\n"
        "  PixelShader = compile ps_3_0 ps_main();\n"
        "} }\n";
    Compiler compiler;
    compiler.setTargetLanguage(Compiler::kLanguageTypeSPIRV);
    compiler.setValidationEnabled(true);
    Compiler::EffectProduct product;
    REQUIRE(compiler.compile(std::string(src), "validated.fx", product));
    REQUIRE(product.numValidatedPasses == product.numPasses);
    REQUIRE(product.sink.validator.empty());
}

TEST_CASE("fx9next generated MSL passes Apple Metal compiler")
{
    const char *src =
        "float4 vs_main(float4 position : POSITION) : POSITION { return position; }\n"
        "float4 ps_main() : COLOR0 { return float4(1, 1, 1, 1); }\n"
        "technique t { pass p {\n"
        "  VertexShader = compile vs_3_0 vs_main();\n"
        "  PixelShader = compile ps_3_0 ps_main();\n"
        "} }\n";
    Compiler compiler;
    compiler.setTargetLanguage(Compiler::kLanguageTypeMSL);
    Compiler::EffectProduct product;
    REQUIRE(compiler.compile(std::string(src), "metal.fx", product));
    Fx9__Effect__Effect *effect =
        fx9__effect__effect__unpack(nullptr, product.message.size(), product.message.data());
    REQUIRE(effect != nullptr);
    REQUIRE(effect->techniques[0]->passes[0]->vertex_shader->msl != nullptr);
    REQUIRE(effect->techniques[0]->passes[0]->pixel_shader->msl != nullptr);
    REQUIRE(compileMetalSource(effect->techniques[0]->passes[0]->vertex_shader->msl, "vertex shader"));
    REQUIRE(compileMetalSource(effect->techniques[0]->passes[0]->pixel_shader->msl, "pixel shader"));
    fx9__effect__effect__free_unpacked(effect, nullptr);
}

TEST_CASE("fx9next compiles ray-mmd to MSL")
{
    static const char *kFiles[] = { "ray-mmd-1.5.2/Main/main.fx", "ray-mmd-1.5.2/ray.fx",
        "ray-mmd-1.5.2/Materials/Hair/material_hair.fx",
        "ray-mmd-1.5.2/Fog/AtmosphericFog/atmospheric_fog.fx",
        "ray-mmd-1.5.2/Extension/ColorGrading/ColorGrading.fx" };
    const std::string name = GENERATE(from_range(std::vector<std::string>(kFiles, kFiles + 5)));
    const std::string path = std::string(FX9NEXT_TEST_EFFECT_FIXTURES_PATH) + "/../../../../MME/" + name;
    FILE *fp = std::fopen(path.c_str(), "rb");
    if (!fp) {
        WARN("missing " + path);
        return;
    }
    std::fclose(fp);
    Compiler compiler;
    compiler.setTargetLanguage(Compiler::kLanguageTypeMSL);
    compiler.setDefineMacro("NANOEM", "1");
    compiler.setDefineMacro("FOG_ENABLE", "0");
    Compiler::EffectProduct product;
    const bool ok = compiler.compile(path.c_str(), product);
    INFO(path);
    INFO(product.sink.info);
    INFO(product.sink.builder);
    if (!product.sink.translator.empty()) {
        INFO(*product.sink.translator.begin());
    }
    CHECK(ok);
    CHECK(product.numPasses > 0);
    CHECK(product.numCompiledPasses == product.numPasses);
    Fx9__Effect__Effect *effect =
        fx9__effect__effect__unpack(nullptr, product.message.size(), product.message.data());
    REQUIRE(effect != nullptr);
    for (size_t ti = 0; ti < effect->n_techniques; ti++) {
        for (size_t pi = 0; pi < effect->techniques[ti]->n_passes; pi++) {
            Fx9__Effect__Pass *pass = effect->techniques[ti]->passes[pi];
            REQUIRE(pass->vertex_shader->msl != nullptr);
            REQUIRE(pass->pixel_shader->msl != nullptr);
            CHECK(compileMetalSource(pass->vertex_shader->msl, "ray-mmd vertex shader"));
            CHECK(compileMetalSource(pass->pixel_shader->msl, "ray-mmd pixel shader"));
        }
    }
    fx9__effect__effect__free_unpacked(effect, nullptr);
}

TEST_CASE("fx9next bakes alpha test into pixel shader")
{
    Compiler compiler;
    compiler.setTargetLanguage(kLanguageTypeGLSL);
    Compiler::EffectProduct product;
    const std::string path = std::string(FX9NEXT_TEST_EFFECT_FIXTURES_PATH) + "/corpus/06_alpha_test_states.fx";
    REQUIRE(compiler.compile(path.c_str(), product));
    Fx9__Effect__Effect *effect =
        fx9__effect__effect__unpack(nullptr, product.message.size(), product.message.data());
    REQUIRE(effect != nullptr);
    REQUIRE(effect->n_techniques > 0);
    REQUIRE(effect->techniques[0]->n_passes > 0);
    const char *ps = effect->techniques[0]->passes[0]->pixel_shader->glsl;
    REQUIRE(ps != nullptr);
    const std::string body(ps);
    REQUIRE(body.find("discard") != std::string::npos);
    fx9__effect__effect__free_unpacked(effect, nullptr);
}

TEST_CASE("fx9next flattens struct entry IO")
{
    Compiler compiler;
    compiler.setTargetLanguage(kLanguageTypeGLSL);
    Compiler::EffectProduct product;
    const std::string path = std::string(FX9NEXT_TEST_EFFECT_FIXTURES_PATH) + "/corpus/19_struct_io.fx";
    REQUIRE(compiler.compile(path.c_str(), product));
    INFO(product.sink.info);
    INFO(product.sink.builder);
    if (!product.sink.translator.empty()) {
        INFO(*product.sink.translator.begin());
    }
    REQUIRE(product.numCompiledPasses == 1);
    Fx9__Effect__Effect *effect =
        fx9__effect__effect__unpack(nullptr, product.message.size(), product.message.data());
    REQUIRE(effect != nullptr);
    const char *vs = effect->techniques[0]->passes[0]->vertex_shader->glsl;
    REQUIRE(vs != nullptr);
    REQUIRE(effect->techniques[0]->passes[0]->vertex_shader->n_inputs == 1);
    REQUIRE(std::string(effect->techniques[0]->passes[0]->vertex_shader->inputs[0]->name) == "input");
    const std::string body(vs);
    REQUIRE(body.find("gl_Position") != std::string::npos);
    fx9__effect__effect__free_unpacked(effect, nullptr);
}

TEST_CASE("fx9next emits nested struct entry IO to MSL")
{
    const char *src =
        "struct Inner { float2 uv : TEXCOORD0; };\n"
        "struct Outer { float4 position : POSITION; Inner detail; };\n"
        "Outer vs_main(float4 position : POSITION) { Outer output = (Outer) 0; output.position = position; "
        "output.detail.uv = float2(0.25, 0.75); "
        "return output; }\n"
        "float4 ps_main(float2 uv : TEXCOORD0) : COLOR0 { return float4(uv, 0, 1); }\n"
        "technique t { pass p { VertexShader = compile vs_3_0 vs_main(); "
        "PixelShader = compile ps_3_0 ps_main(); } }\n";
    Compiler compiler;
    compiler.setTargetLanguage(kLanguageTypeMSL);
    Compiler::EffectProduct product;
    REQUIRE(compiler.compile(std::string(src), "nested-struct.fx", product));
    Fx9__Effect__Effect *effect =
        fx9__effect__effect__unpack(nullptr, product.message.size(), product.message.data());
    REQUIRE(effect != nullptr);
    REQUIRE(effect->techniques[0]->passes[0]->vertex_shader->msl != nullptr);
    REQUIRE(compileMetalSource(effect->techniques[0]->passes[0]->vertex_shader->msl, "nested struct vertex shader"));
    fx9__effect__effect__free_unpacked(effect, nullptr);
}

TEST_CASE("fx9next matrix swizzle and mul")
{
    Compiler compiler;
    compiler.setTargetLanguage(kLanguageTypeGLSL);
    Compiler::EffectProduct product;
    const std::string path = std::string(FX9NEXT_TEST_EFFECT_FIXTURES_PATH) + "/corpus/16_matrix_swizzle.fx";
    REQUIRE(compiler.compile(path.c_str(), product));
    INFO(product.sink.info);
    INFO(product.sink.builder);
    if (!product.sink.translator.empty()) {
        INFO(*product.sink.translator.begin());
    }
    REQUIRE(product.numCompiledPasses == 1);
    Fx9__Effect__Effect *effect =
        fx9__effect__effect__unpack(nullptr, product.message.size(), product.message.data());
    REQUIRE(effect != nullptr);
    const char *vs = effect->techniques[0]->passes[0]->vertex_shader->glsl;
    REQUIRE(vs != nullptr);
    const std::string body(vs);
    REQUIRE((body.find("mat3") != std::string::npos || body.find("mat4") != std::string::npos ||
        body.find("*") != std::string::npos));
    fx9__effect__effect__free_unpacked(effect, nullptr);
}

TEST_CASE("fx9next indexes arrays and runs while")
{
    const char *src =
        "float4 vs_main(float4 position : POSITION) : POSITION { return position; }\n"
        "float4 ps_main() : COLOR0 {\n"
        "  float w[3] = { 0.2, 0.3, 0.5 };\n"
        "  float acc = 0;\n"
        "  int i = 0;\n"
        "  while (i < 3) { acc += w[i]; i++; }\n"
        "  return float4(acc, w[1], 0, 1);\n"
        "}\n"
        "technique t { pass p {\n"
        "  VertexShader = compile vs_3_0 vs_main();\n"
        "  PixelShader = compile ps_3_0 ps_main();\n"
        "} }\n";
    Compiler compiler;
    compiler.setTargetLanguage(kLanguageTypeGLSL);
    Compiler::EffectProduct product;
    REQUIRE(compiler.compile(std::string(src), "arr.fx", product));
    INFO(product.sink.info);
    INFO(product.sink.builder);
    if (!product.sink.translator.empty()) {
        INFO(*product.sink.translator.begin());
    }
    REQUIRE(product.numCompiledPasses == 1);
    Fx9__Effect__Effect *effect =
        fx9__effect__effect__unpack(nullptr, product.message.size(), product.message.data());
    REQUIRE(effect != nullptr);
    const char *ps = effect->techniques[0]->passes[0]->pixel_shader->glsl;
    REQUIRE(ps != nullptr);
    const std::string body(ps);
    REQUIRE((body.find("[") != std::string::npos || body.find("0.3") != std::string::npos));
    fx9__effect__effect__free_unpacked(effect, nullptr);
}

TEST_CASE("fx9next emits if and for control flow")
{
    const char *src =
        "float4 vs_main(float4 position : POSITION) : POSITION { return position; }\n"
        "float4 ps_main(float4 color : COLOR0) : COLOR0 {\n"
        "  float4 c = color;\n"
        "  if (c.r > 0.5) { c.r = 1; } else { c.r = 0; }\n"
        "  for (int i = 0; i < 3; i++) { c.g += 0.1; }\n"
        "  return c;\n"
        "}\n"
        "technique t { pass p {\n"
        "  VertexShader = compile vs_3_0 vs_main();\n"
        "  PixelShader = compile ps_3_0 ps_main();\n"
        "} }\n";
    Compiler compiler;
    compiler.setTargetLanguage(kLanguageTypeGLSL);
    Compiler::EffectProduct product;
    REQUIRE(compiler.compile(std::string(src), "cf.fx", product));
    INFO(product.sink.info);
    INFO(product.sink.builder);
    if (!product.sink.translator.empty()) {
        INFO(*product.sink.translator.begin());
    }
    REQUIRE(product.numCompiledPasses == 1);
}

TEST_CASE("fx9next inlines user functions")
{
    const char *src =
        "float4 tint(float4 c) { return c * 0.5; }\n"
        "float4 vs_main(float4 position : POSITION) : POSITION { return position; }\n"
        "float4 ps_main() : COLOR0 { return tint(float4(1, 1, 1, 1)); }\n"
        "technique t { pass p {\n"
        "  VertexShader = compile vs_3_0 vs_main();\n"
        "  PixelShader = compile ps_3_0 ps_main();\n"
        "} }\n";
    Compiler compiler;
    compiler.setTargetLanguage(kLanguageTypeGLSL);
    Compiler::EffectProduct product;
    REQUIRE(compiler.compile(std::string(src), "fn.fx", product));
    Fx9__Effect__Effect *effect =
        fx9__effect__effect__unpack(nullptr, product.message.size(), product.message.data());
    REQUIRE(effect != nullptr);
    const char *ps = effect->techniques[0]->passes[0]->pixel_shader->glsl;
    REQUIRE(ps != nullptr);
    const std::string body(ps);
    REQUIRE(body.find("0.5") != std::string::npos);
    fx9__effect__effect__free_unpacked(effect, nullptr);
}

TEST_CASE("fx9next lowers texM3x3 family")
{
    Compiler compiler;
    compiler.setTargetLanguage(kLanguageTypeGLSL);
    Compiler::EffectProduct product;
    const std::string path = std::string(FX9NEXT_TEST_EFFECT_FIXTURES_PATH) + "/corpus/03_texM3x3.fx";
    REQUIRE(compiler.compile(path.c_str(), product));
    Fx9__Effect__Effect *effect =
        fx9__effect__effect__unpack(nullptr, product.message.size(), product.message.data());
    REQUIRE(effect != nullptr);
    const char *ps = effect->techniques[0]->passes[0]->pixel_shader->glsl;
    REQUIRE(ps != nullptr);
    const std::string body(ps);
    REQUIRE((body.find("texture") != std::string::npos || body.find("texture2D") != std::string::npos));
    fx9__effect__effect__free_unpacked(effect, nullptr);
}

TEST_CASE("fx9next packs semantic parameters")
{
    Compiler compiler;
    compiler.setTargetLanguage(kLanguageTypeGLSL);
    Compiler::EffectProduct product;
    const std::string path = std::string(FX9NEXT_TEST_EFFECT_FIXTURES_PATH) + "/corpus/14_matrix_semantics.fx";
    REQUIRE(compiler.compile(path.c_str(), product));
    Fx9__Effect__Effect *effect =
        fx9__effect__effect__unpack(nullptr, product.message.size(), product.message.data());
    REQUIRE(effect != nullptr);
    bool foundWorld = false;
    for (size_t i = 0; i < effect->n_parameters; i++) {
        if (effect->parameters[i]->semantic && std::string(effect->parameters[i]->semantic) == "WORLD") {
            foundWorld = true;
            REQUIRE(effect->parameters[i]->has_class_common);
            REQUIRE(effect->parameters[i]->class_common == FX9__EFFECT__PARAMETER__CLASS_COMMON__PC_MATRIX_ROWS);
        }
    }
    fx9__effect__effect__free_unpacked(effect, nullptr);
    REQUIRE(foundWorld);
}

TEST_CASE("fx9next compiles dummy pass")
{
    const char *src =
        "float4 vs_main(float4 position : POSITION) : POSITION { return position; }\n"
        "float4 ps_main() : COLOR0 { return float4(1, 1, 1, 1); }\n"
        "technique dummy_pass {\n"
        "  pass RealDraw {\n"
        "    VertexShader = compile vs_3_0 vs_main();\n"
        "    PixelShader = compile ps_3_0 ps_main();\n"
        "  }\n"
        "  pass StateOnly < string Script = \"Draw=Geometry;\"; > {\n"
        "    ZWriteEnable = false;\n"
        "    AlphaBlendEnable = true;\n"
        "  }\n"
        "}\n";
    Compiler compiler;
    Compiler::EffectProduct product;
    const bool ok = compiler.compile(std::string(src), "dummy.fx", product);
    INFO(product.sink.info);
    INFO(product.sink.builder);
    REQUIRE(ok);
    REQUIRE(product.numPasses == 2);
    REQUIRE(product.numCompiledPasses == 2);
}

TEST_CASE("fx9next rejects an effect with an unknown script command")
{
    const char *src =
        "technique invalid < string Script = \"Unsupported=Value;\"; > { pass p { } }\n";
    Compiler compiler;
    Compiler::EffectProduct product;
    REQUIRE_FALSE(compiler.compile(std::string(src), "invalid-script.fx", product));
    REQUIRE(product.sink.info.find("FX9E1002") != std::string::npos);
}

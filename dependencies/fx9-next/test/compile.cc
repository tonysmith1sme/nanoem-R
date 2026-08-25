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
#include <string>
#include <vector>

using namespace fx9next;

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
        "ray-mmd-1.5.2/Materials/Transparent/material_glass.fx" };
    const std::string name = GENERATE(from_range(std::vector<std::string>(kFiles, kFiles + 5)));
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

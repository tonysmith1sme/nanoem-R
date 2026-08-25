/*
   Copyright (c) 2015-2023 hkrn All rights reserved

   This file is licensed under MIT license. for more details, see LICENSE.txt.
 */

#define CATCH_CONFIG_FAST_COMPILE
#include <catch2/catch.hpp>

#include "fx9next/Compiler.h"
#include "fx9next/Parser.h"

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

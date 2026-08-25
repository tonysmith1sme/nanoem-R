/*
   Copyright (c) 2015-2023 hkrn All rights reserved

   This file is licensed under MIT license. for more details, see LICENSE.txt.
 */

#define CATCH_CONFIG_FAST_COMPILE
#include <catch2/catch.hpp>

#include "fx9next/Diagnostics.h"
#include "fx9next/EffectIR.h"
#include "fx9next/Lowering.h"
#include "fx9next/Parser.h"
#include "fx9next/ShaderIR.h"
#include "fx9next/SpirvEmitter.h"

using namespace fx9next;

TEST_CASE("fx9next diagnostics preserve stable source context")
{
    DiagnosticSink sink;
    sink.add(kDiagnosticType, kDiagnosticError, "FX9T1001", SourceLocation("ray.fx", 12, 4), "ps_main",
        "cannot resolve intrinsic");
    REQUIRE(sink.hasErrors());
    REQUIRE(sink.format() == "error type FX9T1001 ray.fx:12:4 [ps_main]: cannot resolve intrinsic\n");
}

TEST_CASE("fx9next shader IR has a canonical semantic dump")
{
    ShaderModuleIR module;
    module.stage = kShaderStageVertex;
    module.entryPoint = "vs_main";
    ShaderParameterIR input;
    input.name = "position";
    input.type = Type::vectorType(kTypeFloat, 4);
    input.semantic = "POSITION";
    input.input = true;
    input.output = false;
    module.inputs.push_back(input);
    REQUIRE(module.canonicalDump() == "vertex vs_main\nin float4 position : POSITION\n");
}

TEST_CASE("fx9next shader IR preserves reachable struct members")
{
    TranslationUnit unit;
    Parser parser;
    std::string parseError;
    DiagnosticSink diagnostics;
    REQUIRE(parser.parse(
        "struct VSOutput { float4 position : POSITION; float2 uv : TEXCOORD0; };\n"
        "VSOutput vs_main(float4 position : POSITION) { VSOutput output = (VSOutput) 0; "
        "output.position = position; return output; }\n"
        "technique t { pass p { VertexShader = compile vs_3_0 vs_main(); } }\n",
        "struct.fx", unit, parseError));
    REQUIRE(parseError.empty());
    const Function *function = nullptr;
    for (std::vector<Function>::const_iterator it = unit.functions.begin(); it != unit.functions.end(); ++it) {
        if (it->name == "vs_main") {
            function = &*it;
            break;
        }
    }
    REQUIRE(function != nullptr);
    (void) function;
    std::vector<ShaderModuleIR> shaders;
    EffectModuleIR effect;
    REQUIRE(Lowering().lower(unit, shaders, effect, diagnostics));
    REQUIRE(shaders.size() == 1);
    REQUIRE(shaders[0].structs.size() == 1);
    REQUIRE(shaders[0].structs[0].name == "VSOutput");
    REQUIRE(shaders[0].structs[0].members.size() == 2);
    REQUIRE(shaders[0].canonicalDump().find("position") != std::string::npos);
    REQUIRE(shaders[0].canonicalDump().find("TEXCOORD0") != std::string::npos);
}

TEST_CASE("fx9next effect IR preserves resource and pass order")
{
    EffectModuleIR module;
    EffectResourceIR resource;
    resource.name = "Gbuffer2RT";
    resource.type = "RENDERCOLORTARGET";
    resource.format = "A8R8G8B8";
    resource.mipLevels = 1;
    resource.shared = false;
    module.resources.push_back(resource);
    EffectTechniqueIR technique;
    technique.name = "DeferredLighting";
    EffectPassIR pass;
    pass.name = "ShadingOpacity";
    pass.renderStates.push_back(std::make_pair("ZEnable", "false"));
    technique.passes.push_back(pass);
    module.techniques.push_back(technique);
    REQUIRE(module.canonicalDump() ==
        "resource RENDERCOLORTARGET Gbuffer2RT A8R8G8B8 1 local\n"
        "technique DeferredLighting\n"
        "pass ShadingOpacity\n"
        "state ZEnable=false\n");
}

TEST_CASE("fx9next lowers parsed effect scripts and shader entries")
{
    const char *source =
        "texture2D Gbuffer2RT : RENDERCOLORTARGET < string Format = \"A8R8G8B8\"; int Miplevels = 1; >;\n"
        "float4 vs_main(float4 position : POSITION) : POSITION { return position; }\n"
        "float4 ps_main() : COLOR0 { return float4(1, 1, 1, 1); }\n"
        "technique DeferredLighting < string Script = \"RenderColorTarget0=Gbuffer2RT;Pass=Shade;\"; > {\n"
        "  pass Shade < string Script = \"Draw=Buffer;ScriptExternal=Color;\"; > {\n"
        "    ZEnable = false;\n"
        "    VertexShader = compile vs_3_0 vs_main();\n"
        "    PixelShader = compile ps_3_0 ps_main();\n"
        "  }\n"
        "}\n";
    TranslationUnit unit;
    std::string error;
    Parser parser;
    REQUIRE(parser.parse(source, "ray-minimal.fx", unit, error));
    Lowering lowering;
    std::vector<ShaderModuleIR> shaders;
    EffectModuleIR effect;
    DiagnosticSink diagnostics;
    REQUIRE(lowering.lower(unit, shaders, effect, diagnostics));
    REQUIRE_FALSE(diagnostics.hasErrors());
    REQUIRE(effect.resources.size() == 1);
    REQUIRE(effect.resources[0].name == "Gbuffer2RT");
    REQUIRE(effect.resources[0].format == "A8R8G8B8");
    REQUIRE(effect.bindings.size() == 1);
    REQUIRE(effect.bindings[0].name == "Gbuffer2RT");
    REQUIRE(effect.bindings[0].registerSet == kEffectRegisterTexture);
    REQUIRE(effect.bindings[0].registerIndex == 0);
    REQUIRE(effect.techniques.size() == 1);
    REQUIRE(effect.techniques[0].script.size() == 2);
    REQUIRE(effect.techniques[0].passes.size() == 1);
    REQUIRE(effect.techniques[0].passes[0].script.size() == 2);
    REQUIRE(shaders.size() == 2);
    REQUIRE(shaders[0].functions[0].body.get() != nullptr);
    REQUIRE(shaders[0].functions[0].body->kind == kShaderStatementBlock);
    REQUIRE(shaders[0].functions[0].body->children.size() == 1);
    REQUIRE(shaders[0].functions[0].body->children[0]->kind == kShaderStatementReturn);
    REQUIRE(shaders[0].functions[0].body->children[0]->expression->type.toString() == "float4");
}

TEST_CASE("fx9next resolves typed shader control flow into IR")
{
    const char *source =
        "float4 ps_main(float4 color : COLOR0) : COLOR0 {\n"
        "  float value = color.r;\n"
        "  if (value > 0.5) { value = 1; }\n"
        "  return float4(value, 0, 0, 1);\n"
        "}\n"
        "technique t { pass p { PixelShader = compile ps_3_0 ps_main(); } }\n";
    TranslationUnit unit;
    std::string error;
    Parser parser;
    REQUIRE(parser.parse(source, "typed-control.fx", unit, error));
    Lowering lowering;
    std::vector<ShaderModuleIR> shaders;
    EffectModuleIR effect;
    DiagnosticSink diagnostics;
    REQUIRE(lowering.lower(unit, shaders, effect, diagnostics));
    REQUIRE(shaders.size() == 1);
    const ShaderStatementIR *const body = shaders[0].functions[0].body.get();
    REQUIRE(body->children.size() == 3);
    REQUIRE(body->children[0]->kind == kShaderStatementVariable);
    REQUIRE(body->children[0]->expression->type.toString() == "float");
    REQUIRE(body->children[1]->kind == kShaderStatementIf);
    REQUIRE(body->children[1]->expression->type.toString() == "bool");
    REQUIRE(body->children[2]->kind == kShaderStatementReturn);
    REQUIRE(body->children[2]->expression->type.toString() == "float4");
}

TEST_CASE("fx9next emits lowered shader IR deterministically")
{
    const char *source =
        "float4 vs_main(float4 position : POSITION) : POSITION { return position; }\n"
        "float4 ps_main() : COLOR0 { return float4(1, 1, 1, 1); }\n"
        "technique t { pass p {\n"
        "  VertexShader = compile vs_3_0 vs_main;\n"
        "  PixelShader = compile ps_3_0 ps_main;\n"
        "} }\n";
    TranslationUnit unit;
    std::string error;
    Parser parser;
    REQUIRE(parser.parse(source, "ir-emit.fx", unit, error));
    Lowering lowering;
    std::vector<ShaderModuleIR> shaders;
    EffectModuleIR effect;
    DiagnosticSink diagnostics;
    REQUIRE(lowering.lower(unit, shaders, effect, diagnostics));
    REQUIRE(shaders.size() == 2);
    std::vector<uint32_t> legacy, lowered;
    REQUIRE(emitFunctionSPIRV(unit, unit.functions[0], kStageVertex, legacy, error));
    REQUIRE(emitShaderSPIRV(unit, effect, shaders[0], lowered, error));
    REQUIRE(lowered == legacy);
}

TEST_CASE("fx9next rejects unknown effect script commands")
{
    TranslationUnit unit;
    Technique technique;
    technique.name = "t";
    Annotation annotation;
    annotation.name = "Script";
    annotation.kind = Annotation::kAnnString;
    annotation.sval = "  Unsupported = Value;";
    technique.annotations.push_back(annotation);
    unit.techniques.push_back(technique);
    Lowering lowering;
    std::vector<ShaderModuleIR> shaders;
    EffectModuleIR effect;
    DiagnosticSink diagnostics;
    REQUIRE_FALSE(lowering.lower(unit, shaders, effect, diagnostics));
    REQUIRE(diagnostics.hasErrors());
    REQUIRE(diagnostics.diagnostics()[0].code == "FX9E1002");
}

TEST_CASE("fx9next rejects overlapping DX9 register bindings")
{
    const char *source =
        "float4 first : register(c0);\n"
        "float4 second : register(c0);\n"
        "float4 ps_main() : COLOR0 { return first + second; }\n"
        "technique t { pass p { PixelShader = compile ps_3_0 ps_main(); } }\n";
    TranslationUnit unit;
    std::string error;
    Parser parser;
    REQUIRE(parser.parse(source, "overlap.fx", unit, error));
    Lowering lowering;
    std::vector<ShaderModuleIR> shaders;
    EffectModuleIR effect;
    DiagnosticSink diagnostics;
    REQUIRE_FALSE(lowering.lower(unit, shaders, effect, diagnostics));
    REQUIRE(diagnostics.diagnostics()[0].code == "FX9T1004");
}

TEST_CASE("fx9next validates SPIR-V instruction boundaries")
{
    std::string error;
    REQUIRE(validateSPIRV(std::vector<uint32_t>{ 0x07230203, 0x00010000, 0, 1, 0 }, error));
    REQUIRE_FALSE(validateSPIRV(std::vector<uint32_t>{ 0x07230203, 0x00010000, 0, 1, 0, 0 }, error));
    REQUIRE(error.find("invalid SPIR-V instruction") != std::string::npos);
}

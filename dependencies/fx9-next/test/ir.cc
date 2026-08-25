/*
   Copyright (c) 2015-2023 hkrn All rights reserved

   This file is licensed under MIT license. for more details, see LICENSE.txt.
 */

#define CATCH_CONFIG_FAST_COMPILE
#include <catch2/catch.hpp>

#include "fx9next/Diagnostics.h"
#include "fx9next/EffectIR.h"
#include "fx9next/ShaderIR.h"

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

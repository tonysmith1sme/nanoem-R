/*
   Copyright (c) 2015-2023 hkrn All rights reserved

   This file is licensed under MIT license. for more details, see LICENSE.txt.
 */

#include "fx9next/Lowering.h"

#include <sstream>

namespace fx9next {
namespace {

const Function *
findFunction(const TranslationUnit &unit, const std::string &name)
{
    for (std::vector<Function>::const_iterator it = unit.functions.begin(); it != unit.functions.end(); ++it) {
        if (it->name == name) {
            return &*it;
        }
    }
    return nullptr;
}

const Annotation *
findAnnotation(const std::vector<Annotation> &annotations, const char *name)
{
    for (std::vector<Annotation>::const_iterator it = annotations.begin(); it != annotations.end(); ++it) {
        if (it->name == name) {
            return &*it;
        }
    }
    return nullptr;
}

ShaderModuleIR
makeShader(const Function &function, ShaderStage stage)
{
    ShaderModuleIR shader;
    shader.stage = stage;
    shader.entryPoint = function.name;
    for (std::vector<Parameter>::const_iterator it = function.params.begin(); it != function.params.end(); ++it) {
        ShaderParameterIR parameter;
        parameter.name = it->name;
        parameter.type = it->type;
        parameter.semantic = it->semantic;
        parameter.input = !it->isOut;
        parameter.output = it->isOut;
        if (parameter.input) {
            shader.inputs.push_back(parameter);
        }
        else {
            shader.outputs.push_back(parameter);
        }
    }
    if (!function.returnType.isVoid()) {
        ShaderParameterIR output;
        output.name = "$return";
        output.type = function.returnType;
        output.semantic = function.returnSemantic;
        output.input = false;
        output.output = true;
        shader.outputs.push_back(output);
    }
    ShaderFunctionIR declaration;
    declaration.name = function.name;
    declaration.returnType = function.returnType;
    for (std::vector<Parameter>::const_iterator it = function.params.begin(); it != function.params.end(); ++it) {
        ShaderParameterIR parameter;
        parameter.name = it->name;
        parameter.type = it->type;
        parameter.semantic = it->semantic;
        parameter.input = !it->isOut;
        parameter.output = it->isOut;
        declaration.parameters.push_back(parameter);
    }
    shader.functions.push_back(declaration);
    return shader;
}

} /* namespace anonymous */

bool
Lowering::lowerScript(const std::string &text, std::vector<EffectScriptCommandIR> &commands,
    const std::string &entity, DiagnosticSink &diagnostics) const
{
    size_t offset = 0;
    while (offset < text.size()) {
        const size_t end = text.find(';', offset);
        if (end == std::string::npos) {
            if (text.find_first_not_of(" \t\r\n", offset) != std::string::npos) {
                diagnostics.add(kDiagnosticEffect, kDiagnosticError, "FX9E1001", SourceLocation(), entity,
                    "script command is missing a terminating semicolon");
                return false;
            }
            break;
        }
        const std::string command = text.substr(offset, end - offset);
        offset = end + 1;
        const size_t equals = command.find('=');
        if (equals == std::string::npos) {
            continue;
        }
        const std::string key = command.substr(0, equals);
        const std::string value = command.substr(equals + 1);
        EffectScriptCommandIR lowered;
        lowered.name = key;
        lowered.value = value;
        lowered.index = 0;
        if (key == "RenderColorTarget" || key == "RenderColorTarget0") {
            lowered.type = kEffectScriptRenderColorTarget;
        }
        else if (key.size() == 18 && key.compare(0, 17, "RenderColorTarget") == 0 && key[17] >= '1' &&
            key[17] <= '3') {
            lowered.type = kEffectScriptRenderColorTarget;
            lowered.index = key[17] - '0';
        }
        else if (key == "RenderDepthStencilTarget") {
            lowered.type = kEffectScriptRenderDepthStencilTarget;
        }
        else if (key == "Clear" || key == "ClearSetColor" || key == "ClearSetDepth" || key == "ClearSetStencil") {
            lowered.type = kEffectScriptClear;
        }
        else if (key == "Pass") {
            lowered.type = kEffectScriptPass;
        }
        else if (key == "Draw") {
            lowered.type = kEffectScriptDraw;
        }
        else if (key == "ScriptExternal") {
            lowered.type = kEffectScriptExternal;
        }
        else if (key == "LoopByCount" || key == "LoopGetIndex" || key == "LoopEnd") {
            lowered.type = kEffectScriptLoop;
        }
        else {
            diagnostics.add(kDiagnosticEffect, kDiagnosticError, "FX9E1002", SourceLocation(), entity,
                "unknown script command: " + key);
            return false;
        }
        commands.push_back(lowered);
    }
    return true;
}

bool
Lowering::lower(const TranslationUnit &unit, std::vector<ShaderModuleIR> &shaders, EffectModuleIR &effect,
    DiagnosticSink &diagnostics) const
{
    shaders.clear();
    effect.resources.clear();
    effect.techniques.clear();
    diagnostics.clear();
    for (std::vector<Variable>::const_iterator variable = unit.variables.begin(); variable != unit.variables.end();
         ++variable) {
        if (variable->type.kind == kTypeTexture) {
            EffectResourceIR resource;
            resource.name = variable->name;
            resource.type = variable->semantic.empty() ? "TEXTURE" : variable->semantic;
            resource.format = "";
            resource.mipLevels = 1;
            resource.shared = false;
            const Annotation *format = findAnnotation(variable->annotations, "Format");
            const Annotation *mipLevels = findAnnotation(variable->annotations, "Miplevels");
            if (format && format->kind == Annotation::kAnnString) {
                resource.format = format->sval;
            }
            if (mipLevels && mipLevels->kind == Annotation::kAnnInt) {
                resource.mipLevels = mipLevels->ival;
            }
            effect.resources.push_back(resource);
        }
    }
    for (std::vector<Technique>::const_iterator technique = unit.techniques.begin(); technique != unit.techniques.end();
         ++technique) {
        EffectTechniqueIR loweredTechnique;
        loweredTechnique.name = technique->name;
        const Annotation *techniqueScript = findAnnotation(technique->annotations, "Script");
        if (techniqueScript && techniqueScript->kind == Annotation::kAnnString &&
            !lowerScript(techniqueScript->sval, loweredTechnique.script, technique->name, diagnostics)) {
            return false;
        }
        for (std::vector<Pass>::const_iterator pass = technique->passes.begin(); pass != technique->passes.end(); ++pass) {
            EffectPassIR loweredPass;
            loweredPass.name = pass->name;
            const Annotation *passScript = findAnnotation(pass->annotations, "Script");
            if (passScript && passScript->kind == Annotation::kAnnString &&
                !lowerScript(passScript->sval, loweredPass.script, technique->name + "/" + pass->name, diagnostics)) {
                return false;
            }
            for (std::vector<PassState>::const_iterator state = pass->states.begin(); state != pass->states.end(); ++state) {
                if (state->compileProfile.empty()) {
                    loweredPass.renderStates.push_back(std::make_pair(state->name, state->value));
                }
            }
            if (!pass->vsEntry.empty()) {
                const Function *function = findFunction(unit, pass->vsEntry);
                if (!function) {
                    diagnostics.add(kDiagnosticType, kDiagnosticError, "FX9T1002", SourceLocation(), pass->name,
                        "vertex shader entry does not resolve: " + pass->vsEntry);
                    return false;
                }
                ShaderModuleIR shader = makeShader(*function, kShaderStageVertex);
                loweredPass.shaders.push_back(shader);
                shaders.push_back(shader);
            }
            if (!pass->psEntry.empty()) {
                const Function *function = findFunction(unit, pass->psEntry);
                if (!function) {
                    diagnostics.add(kDiagnosticType, kDiagnosticError, "FX9T1003", SourceLocation(), pass->name,
                        "pixel shader entry does not resolve: " + pass->psEntry);
                    return false;
                }
                ShaderModuleIR shader = makeShader(*function, kShaderStagePixel);
                loweredPass.shaders.push_back(shader);
                shaders.push_back(shader);
            }
            loweredTechnique.passes.push_back(loweredPass);
        }
        effect.techniques.push_back(loweredTechnique);
    }
    return !diagnostics.hasErrors();
}

} /* namespace fx9next */

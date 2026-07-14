/*
   Copyright (c) 2015-2023 hkrn All rights reserved

   This file is licensed under MIT license. for more details, see LICENSE.txt.
 */

#pragma once
#ifndef FX9_SHADER_CROSS_TRANSLATOR_H_
#define FX9_SHADER_CROSS_TRANSLATOR_H_

#include <functional>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace fx9 {
namespace translation {

/* glslang language stage id – avoid redeclaring EShLanguage (C typedef enum). */
using ShaderStageLanguage = int;
using SPIRVWords = std::vector<uint32_t>;
using SamplerBindingMap = std::unordered_map<std::string, int>;
using AttributeNameMap = std::unordered_map<uint32_t, std::string>;
using BuiltInLocationMap = std::unordered_map<std::string, int>;
using ErrorSink = std::unordered_set<std::string>;

/**
 * Host callbacks supplied by Compiler so the cross-translator stays free of
 * Optimizer / Parser / protobuf dependencies.
 */
struct CrossHostServices {
    /* Optional SPIRV-Tools optimize pass. Must always fill outWords. */
    std::function<void(const SPIRVWords &inWords, SPIRVWords &outWords, ErrorSink &errors)> optimize;
    /* Snapshot interface variable names before optimize/rename. */
    std::function<void(const SPIRVWords &words, AttributeNameMap &attributes)> saveAttributes;
    /* Restore interface / uniform buffer names after SPIRV-Cross setup. */
    std::function<void(ShaderStageLanguage language, const AttributeNameMap &attributes, void *spirvCrossCompiler)> restoreAttributes;
};

struct GLSLBackendOptions {
    bool es = false;
    int version = 330;
};

struct HLSLBackendOptions {
    int shaderModel = 41; /* SM 4.1 – matches emapp D3DCompile vs_4_1/ps_4_1 */
};

struct MSLBackendOptions {
    int major = 2;
    int minor = 0;
    std::string entryPoint = "fx9_metal_main";
    std::string uniformBufferName = "nanoem_uniforms";
    /* Maps original interface variable name -> stable location for VS out / PS in. */
    BuiltInLocationMap interfaceLocations;
};

struct CrossTranslateRequest {
    const SPIRVWords *vertexSPIRV = nullptr;
    const SPIRVWords *fragmentSPIRV = nullptr;
    const SamplerBindingMap *vertexSamplers = nullptr;
    const SamplerBindingMap *fragmentSamplers = nullptr;
};

struct CrossTranslateResult {
    std::string vertexSource;
    std::string fragmentSource;
    bool succeeded = false;
};

/* Apply deterministic SPIRV-Cross options for each backend. */
void configureGLSLBackend(void *compilerGLSL, const GLSLBackendOptions &options);
void configureHLSLBackend(void *compilerHLSL, const HLSLBackendOptions &options);
void configureMSLBackend(void *compilerMSL, const MSLBackendOptions &options);

const char *metalShaderPreamble();

/**
 * DX9 HLSL IR (via glslang SPIR-V) -> target shading language translators.
 * These replace the former Compiler::*PassShader::translate bodies.
 */
bool translateToGLSL(const CrossTranslateRequest &request, const GLSLBackendOptions &options,
    const CrossHostServices &host, CrossTranslateResult &result, ErrorSink &errors);

bool translateToHLSL(const CrossTranslateRequest &request, const HLSLBackendOptions &options,
    const CrossHostServices &host, CrossTranslateResult &result, ErrorSink &errors);

bool translateToMSL(const CrossTranslateRequest &request, const MSLBackendOptions &options,
    const CrossHostServices &host, CrossTranslateResult &result, ErrorSink &errors);

} /* namespace translation */
} /* namespace fx9 */

#endif /* FX9_SHADER_CROSS_TRANSLATOR_H_ */

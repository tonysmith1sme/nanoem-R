/*
   Copyright (c) 2015-2023 hkrn All rights reserved

   This file is licensed under MIT license. for more details, see LICENSE.txt.
 */

#pragma once
#ifndef FX9_GRAPHICS_API_TRANSLATOR_H_
#define FX9_GRAPHICS_API_TRANSLATOR_H_

#include <functional>
#include <stdint.h>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace fx9 {
namespace graphics {

/**
 * Unified graphics-API translation layer.
 *
 * Replaces the former ShaderCrossTranslator split (GLSL/HLSL/MSL-only) with a
 * single adapter model that owns every target used by MME effects:
 *
 *   OpenGL   -> GLSL source          (protobuf BODY_GLSL)
 *   OpenGLES -> ESSL source          (protobuf BODY_GLSL, es profile)
 *   DirectX  -> HLSL SM4.1 source    (protobuf BODY_HLSL)
 *   Metal    -> MSL source           (protobuf BODY_MSL)
 *   Vulkan   -> SPIR-V binary        (protobuf BODY_SPIRV)
 *
 * Input is always glslang-produced SPIR-V from DX9 HLSL effect IR.
 * Host services keep Optimizer / attribute rename out of this module.
 */

enum class GraphicsAPI {
    OpenGL = 0,
    OpenGLES,
    DirectX,
    Metal,
    Vulkan
};

/* glslang stage id – avoid redeclaring EShLanguage (C typedef enum). */
using ShaderStageLanguage = int;
using SPIRVWords = std::vector<uint32_t>;
using SamplerBindingMap = std::unordered_map<std::string, int>;
using AttributeNameMap = std::unordered_map<uint32_t, std::string>;
using BuiltInLocationMap = std::unordered_map<std::string, int>;
using ErrorSink = std::unordered_set<std::string>;

struct HostServices {
    std::function<void(const SPIRVWords &inWords, SPIRVWords &outWords, ErrorSink &errors)> optimize;
    std::function<void(const SPIRVWords &words, AttributeNameMap &attributes)> saveAttributes;
    std::function<void(ShaderStageLanguage language, const AttributeNameMap &attributes, void *spirvCrossCompiler)>
        restoreAttributes;
};

struct OpenGLOptions {
    bool es = false;
    int version = 330;
};

struct DirectXOptions {
    int shaderModel = 41; /* SM 4.1 – matches emapp D3DCompile vs_4_1/ps_4_1 */
};

struct MetalOptions {
    int major = 2;
    int minor = 0;
    std::string entryPoint = "fx9_metal_main";
    std::string uniformBufferName = "nanoem_uniforms";
    BuiltInLocationMap interfaceLocations;
};

struct VulkanOptions {
    /* Descriptor set for sampled images / UBOs (MME register model uses set 0). */
    uint32_t descriptorSet = 0;
    /* When true, rewrite Binding decorations from sampler maps before packing. */
    bool applySamplerBindings = true;
};

struct TranslateRequest {
    const SPIRVWords *vertexSPIRV = nullptr;
    const SPIRVWords *fragmentSPIRV = nullptr;
    const SamplerBindingMap *vertexSamplers = nullptr;
    const SamplerBindingMap *fragmentSamplers = nullptr;
};

/**
 * Unified translation result.
 * Source backends fill vertexSource/fragmentSource.
 * Vulkan fills vertexSPIRV/fragmentSPIRV binary words.
 */
struct TranslateResult {
    std::string vertexSource;
    std::string fragmentSource;
    SPIRVWords vertexSPIRV;
    SPIRVWords fragmentSPIRV;
    GraphicsAPI api = GraphicsAPI::OpenGL;
    bool succeeded = false;
};

struct BackendOptions {
    OpenGLOptions opengl;
    DirectXOptions directx;
    MetalOptions metal;
    VulkanOptions vulkan;
};

const char *graphicsAPIName(GraphicsAPI api);
const char *metalShaderPreamble();

/* Low-level SPIRV-Cross option helpers (void* avoids leaking spirv-cross headers). */
void configureOpenGLBackend(void *compilerGLSL, const OpenGLOptions &options);
void configureDirectXBackend(void *compilerHLSL, const DirectXOptions &options);
void configureMetalBackend(void *compilerMSL, const MetalOptions &options);

/**
 * Translate glslang SPIR-V into the selected graphics API artifact.
 * Vulkan returns binary SPIR-V; all others return shading-language source.
 */
bool translate(GraphicsAPI api, const TranslateRequest &request, const BackendOptions &options,
    const HostServices &host, TranslateResult &result, ErrorSink &errors);

bool translateOpenGL(const TranslateRequest &request, const OpenGLOptions &options, const HostServices &host,
    TranslateResult &result, ErrorSink &errors);
bool translateOpenGLES(const TranslateRequest &request, const OpenGLOptions &options, const HostServices &host,
    TranslateResult &result, ErrorSink &errors);
bool translateDirectX(const TranslateRequest &request, const DirectXOptions &options, const HostServices &host,
    TranslateResult &result, ErrorSink &errors);
bool translateMetal(const TranslateRequest &request, const MetalOptions &options, const HostServices &host,
    TranslateResult &result, ErrorSink &errors);
bool translateVulkan(const TranslateRequest &request, const VulkanOptions &options, const HostServices &host,
    TranslateResult &result, ErrorSink &errors);

} /* namespace graphics */
} /* namespace fx9 */

#endif /* FX9_GRAPHICS_API_TRANSLATOR_H_ */

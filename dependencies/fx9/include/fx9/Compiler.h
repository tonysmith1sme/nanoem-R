/*
   Copyright (c) 2015-2023 hkrn All rights reserved

   This file is licensed under MIT license. for more details, see LICENSE.txt.
 */

#pragma once
#ifndef FX9_COMPILER_H_
#define FX9_COMPILER_H_

#include <memory>
#include <stdint.h>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "Parser.h"
#include "EffectPipeline.h"

namespace fx9 {

/**
 * ABI-stable facade over the modular EffectPipeline.
 *
 * Historical monolithic parse/compile workflow has been deleted from this
 * type. All MME effect work (source prep, Lemon frontend, pass backends,
 * protobuf emission) lives in pipeline::EffectPipeline.
 */
class Compiler {
public:
    using LanguageType = pipeline::EffectPipeline::LanguageType;
    using EffectProduct = pipeline::EffectPipeline::EffectProduct;
    static constexpr LanguageType kLanguageTypeFirstEnum = pipeline::EffectPipeline::kLanguageTypeFirstEnum;
    static constexpr LanguageType kLanguageTypeGLSL = pipeline::EffectPipeline::kLanguageTypeGLSL;
    static constexpr LanguageType kLanguageTypeESSL = pipeline::EffectPipeline::kLanguageTypeESSL;
    static constexpr LanguageType kLanguageTypeHLSL = pipeline::EffectPipeline::kLanguageTypeHLSL;
    static constexpr LanguageType kLanguageTypeMSL = pipeline::EffectPipeline::kLanguageTypeMSL;
    static constexpr LanguageType kLanguageTypeSPIRV = pipeline::EffectPipeline::kLanguageTypeSPIRV;
    static constexpr LanguageType kLanguageTypeMaxEnum = pipeline::EffectPipeline::kLanguageTypeMaxEnum;

    static void initialize();
    static void terminate();

    Compiler(EProfile profile, EShMessages messages);
    ~Compiler();

    bool compile(const char *path, EffectProduct &effectProduct);
    bool compile(const std::string &source, const char *filename, EffectProduct &effectProduct);
    void addIncludeSource(const std::string &filePath, const std::string &sourceData);
    void setDefineMacro(const std::string &key, const std::string &value);
    bool containsDefineMacro(const std::string &key) const;
    void removeDefineMacro(const std::string &key);

    ParserContext::BuiltInLocationMap vertexShaderInputLocations() const;
    void setVertexShaderInputLocations(const ParserContext::BuiltInLocationMap &value);
    ParserContext::BuiltInVariableMap vertexShaderInputVariables() const;
    void setVertexShaderInputVariables(const ParserContext::BuiltInVariableMap &value);
    ParserContext::BuiltInVariableMap pixelShaderInputVariables() const;
    void setPixelShaderInputVariables(const ParserContext::BuiltInVariableMap &value);
    std::string metalShaderEntryPoint() const;
    void setMetalShaderEntryPoint(const std::string &value);
    std::string metalShaderUniformBufferName() const;
    void setMetalShaderUniformBufferName(const std::string &value);
    LanguageType targetLanguage() const;
    void setTargetLanguage(LanguageType value);
    int version() const;
    void setVersion(int value);
    bool isOptimizeEnabled() const;
    void setOptimizeEnabled(bool value);
    bool isValidationEnabled() const;
    void setValidationEnabled(bool value);

private:
    std::unique_ptr<pipeline::EffectPipeline> m_pipeline;
};

} /* namespace fx9 */

#endif /* FX9_COMPILER_H_ */

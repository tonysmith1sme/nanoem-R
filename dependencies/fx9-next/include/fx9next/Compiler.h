/*
   Copyright (c) 2015-2023 hkrn All rights reserved

   This file is licensed under MIT license. for more details, see LICENSE.txt.
 */

#pragma once
#ifndef FX9NEXT_COMPILER_H_
#define FX9NEXT_COMPILER_H_

#include <memory>
#include <string>

#include "fx9next/Product.h"

namespace fx9next {

class Pipeline;

class Compiler {
public:
    using LanguageType = fx9next::LanguageType;
    using EffectProduct = fx9next::EffectProduct;
    static constexpr LanguageType kLanguageTypeFirstEnum = fx9next::kLanguageTypeFirstEnum;
    static constexpr LanguageType kLanguageTypeGLSL = fx9next::kLanguageTypeGLSL;
    static constexpr LanguageType kLanguageTypeESSL = fx9next::kLanguageTypeESSL;
    static constexpr LanguageType kLanguageTypeHLSL = fx9next::kLanguageTypeHLSL;
    static constexpr LanguageType kLanguageTypeMSL = fx9next::kLanguageTypeMSL;
    static constexpr LanguageType kLanguageTypeSPIRV = fx9next::kLanguageTypeSPIRV;
    static constexpr LanguageType kLanguageTypeMaxEnum = fx9next::kLanguageTypeMaxEnum;

    static void initialize();
    static void terminate();

    Compiler();
    Compiler(int /*profile*/, int /*messages*/);
    ~Compiler();

    bool compile(const char *path, EffectProduct &effectProduct);
    bool compile(const std::string &source, const char *filename, EffectProduct &effectProduct);
    void addIncludeSource(const std::string &filePath, const std::string &sourceData);
    void setDefineMacro(const std::string &key, const std::string &value);
    bool containsDefineMacro(const std::string &key) const;
    void removeDefineMacro(const std::string &key);

    BuiltInLocationMap vertexShaderInputLocations() const;
    void setVertexShaderInputLocations(const BuiltInLocationMap &value);
    BuiltInVariableMap vertexShaderInputVariables() const;
    void setVertexShaderInputVariables(const BuiltInVariableMap &value);
    BuiltInVariableMap pixelShaderInputVariables() const;
    void setPixelShaderInputVariables(const BuiltInVariableMap &value);
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
    std::unique_ptr<Pipeline> m_pipeline;
};

} /* namespace fx9next */

#endif /* FX9NEXT_COMPILER_H_ */

/*
   Copyright (c) 2015-2023 hkrn All rights reserved

   This file is licensed under MIT license. for more details, see LICENSE.txt.
 */

#pragma once
#ifndef FX9NEXT_PIPELINE_H_
#define FX9NEXT_PIPELINE_H_

#include <string>
#include <unordered_map>

#include "fx9next/Preprocessor.h"
#include "fx9next/Product.h"

namespace fx9next {

class Pipeline {
public:
    Pipeline();
    ~Pipeline();

    bool compile(const char *path, EffectProduct &effectProduct);
    bool compile(const std::string &source, const char *filename, EffectProduct &effectProduct);
    void addIncludeSource(const std::string &filePath, const std::string &sourceData);
    void clearIncludeSources();
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
    Preprocessor m_pp;
    BuiltInLocationMap m_vsInputLocations;
    BuiltInVariableMap m_vsInputVariables;
    BuiltInVariableMap m_psInputVariables;
    std::string m_metalEntry;
    std::string m_metalUbo;
    LanguageType m_language;
    int m_version;
    bool m_optimize;
    bool m_validate;
};

} /* namespace fx9next */

#endif /* FX9NEXT_PIPELINE_H_ */

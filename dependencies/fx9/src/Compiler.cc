/*
   Copyright (c) 2015-2023 hkrn All rights reserved

   This file is licensed under MIT license. for more details, see LICENSE.txt.
 */

#include "fx9/Compiler.h"

#include "glslang/Public/ShaderLang.h"

namespace fx9 {

void
Compiler::initialize()
{
    glslang::InitializeProcess();
}

void
Compiler::terminate()
{
    glslang::FinalizeProcess();
}

Compiler::Compiler(EProfile profile, EShMessages messages)
    : m_pipeline(new pipeline::EffectPipeline(profile, messages))
{
}

Compiler::~Compiler()
{
}

bool
Compiler::compile(const char *path, EffectProduct &effectProduct)
{
    return m_pipeline->compile(path, effectProduct);
}

bool
Compiler::compile(const std::string &source, const char *filename, EffectProduct &effectProduct)
{
    return m_pipeline->compile(source, filename, effectProduct);
}

void
Compiler::addIncludeSource(const std::string &filePath, const std::string &sourceData)
{
    m_pipeline->addIncludeSource(filePath, sourceData);
}

void
Compiler::setDefineMacro(const std::string &key, const std::string &value)
{
    m_pipeline->setDefineMacro(key, value);
}

bool
Compiler::containsDefineMacro(const std::string &key) const
{
    return m_pipeline->containsDefineMacro(key);
}

void
Compiler::removeDefineMacro(const std::string &key)
{
    m_pipeline->removeDefineMacro(key);
}

ParserContext::BuiltInLocationMap
Compiler::vertexShaderInputLocations() const
{
    return m_pipeline->vertexShaderInputLocations();
}

void
Compiler::setVertexShaderInputLocations(const ParserContext::BuiltInLocationMap &value)
{
    m_pipeline->setVertexShaderInputLocations(value);
}

ParserContext::BuiltInVariableMap
Compiler::vertexShaderInputVariables() const
{
    return m_pipeline->vertexShaderInputVariables();
}

void
Compiler::setVertexShaderInputVariables(const ParserContext::BuiltInVariableMap &value)
{
    m_pipeline->setVertexShaderInputVariables(value);
}

ParserContext::BuiltInVariableMap
Compiler::pixelShaderInputVariables() const
{
    return m_pipeline->pixelShaderInputVariables();
}

void
Compiler::setPixelShaderInputVariables(const ParserContext::BuiltInVariableMap &value)
{
    m_pipeline->setPixelShaderInputVariables(value);
}

std::string
Compiler::metalShaderEntryPoint() const
{
    return m_pipeline->metalShaderEntryPoint();
}

void
Compiler::setMetalShaderEntryPoint(const std::string &value)
{
    m_pipeline->setMetalShaderEntryPoint(value);
}

std::string
Compiler::metalShaderUniformBufferName() const
{
    return m_pipeline->metalShaderUniformBufferName();
}

void
Compiler::setMetalShaderUniformBufferName(const std::string &value)
{
    m_pipeline->setMetalShaderUniformBufferName(value);
}

Compiler::LanguageType
Compiler::targetLanguage() const
{
    return m_pipeline->targetLanguage();
}

void
Compiler::setTargetLanguage(LanguageType value)
{
    m_pipeline->setTargetLanguage(value);
}

int
Compiler::version() const
{
    return m_pipeline->version();
}

void
Compiler::setVersion(int value)
{
    m_pipeline->setVersion(value);
}

bool
Compiler::isOptimizeEnabled() const
{
    return m_pipeline->isOptimizeEnabled();
}

void
Compiler::setOptimizeEnabled(bool value)
{
    m_pipeline->setOptimizeEnabled(value);
}

bool
Compiler::isValidationEnabled() const
{
    return m_pipeline->isValidationEnabled();
}

void
Compiler::setValidationEnabled(bool value)
{
    m_pipeline->setValidationEnabled(value);
}

} /* namespace fx9 */

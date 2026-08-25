/*
   Copyright (c) 2015-2023 hkrn All rights reserved

   This file is licensed under MIT license. for more details, see LICENSE.txt.
 */

#include "fx9next/Pipeline.h"

#include "fx9next/Encoding.h"
#include "fx9next/Lowering.h"
#include "fx9next/Parser.h"
#include "fx9next/ProductWriter.h"

#ifdef _WIN32
#include <io.h>
#include <fcntl.h>
#else
#include <fcntl.h>
#include <unistd.h>
#endif

namespace fx9next {
namespace {

bool
loadPath(const char *path, std::string &out)
{
#ifdef _WIN32
    int fd = _open(path, _O_RDONLY | _O_BINARY);
#else
    int fd = open(path, O_RDONLY);
#endif
    if (fd < 0) {
        return false;
    }
#ifdef _WIN32
    long size = _lseek(fd, 0, SEEK_END);
    _lseek(fd, 0, SEEK_SET);
#else
    off_t size = lseek(fd, 0, SEEK_END);
    lseek(fd, 0, SEEK_SET);
#endif
    if (size < 0) {
#ifdef _WIN32
        _close(fd);
#else
        close(fd);
#endif
        return false;
    }
    std::string buffer(static_cast<size_t>(size), '\0');
#ifdef _WIN32
    int n = _read(fd, &buffer[0], static_cast<unsigned int>(size));
    _close(fd);
#else
    ssize_t n = read(fd, &buffer[0], static_cast<size_t>(size));
    close(fd);
#endif
    if (n < 0) {
        return false;
    }
    buffer.resize(static_cast<size_t>(n));
    out = decodeTextSource(buffer.data(), buffer.size());
    return true;
}

} /* namespace anonymous */

Pipeline::Pipeline()
    : m_metalEntry("fx9_metal_main")
    , m_metalUbo("nanoem_uniforms")
    , m_language(kLanguageTypeGLSL)
    , m_version(330)
    , m_optimize(false)
    , m_validate(false)
{
}

Pipeline::~Pipeline()
{
}

bool
Pipeline::compile(const char *path, EffectProduct &effectProduct)
{
    std::string source;
    if (!loadPath(path, source)) {
        effectProduct.sink.info = std::string("cannot open ") + (path ? path : "");
        return false;
    }
    return compile(source, path ? path : "", effectProduct);
}

bool
Pipeline::compile(const std::string &source, const char *filename, EffectProduct &effectProduct)
{
    const char *name = filename ? filename : "";
    std::string prepared, error;
    if (!m_pp.process(source, name, prepared, error)) {
        effectProduct.sink.info = error;
        return false;
    }
    TranslationUnit unit;
    unit.includes = m_pp.includedPaths();
    Parser parser;
    if (!parser.parse(prepared, name, unit, error)) {
        effectProduct.sink.info = error;
        effectProduct.sink.debug = prepared;
        return false;
    }
    std::vector<ShaderModuleIR> shaders;
    EffectModuleIR effect;
    DiagnosticSink diagnostics;
    Lowering lowering;
    if (!lowering.lower(unit, shaders, effect, diagnostics)) {
        effectProduct.sink.info = diagnostics.format();
        effectProduct.sink.debug = prepared;
        return false;
    }
    const bool ok = writeEffectProduct(
        unit, effect, shaders, m_language, m_metalEntry, m_metalUbo, m_version, m_validate, effectProduct);
    if (!ok && effectProduct.sink.info.empty()) {
        effectProduct.sink.info = "passes=" + std::to_string(effectProduct.numPasses) +
            " compiled=" + std::to_string(effectProduct.numCompiledPasses) +
            " functions=" + std::to_string(unit.functions.size()) +
            " techniques=" + std::to_string(unit.techniques.size());
        if (!effectProduct.sink.builder.empty()) {
            effectProduct.sink.info += " builder=" + effectProduct.sink.builder;
        }
        if (!effectProduct.sink.translator.empty()) {
            effectProduct.sink.info += " xlate=" + *effectProduct.sink.translator.begin();
        }
    }
    return ok;
}

void
Pipeline::addIncludeSource(const std::string &filePath, const std::string &sourceData)
{
    m_pp.addIncludeSource(filePath, sourceData);
}

void
Pipeline::setDefineMacro(const std::string &key, const std::string &value)
{
    m_pp.setMacro(key, value);
}

bool
Pipeline::containsDefineMacro(const std::string &key) const
{
    return m_pp.containsMacro(key);
}

void
Pipeline::removeDefineMacro(const std::string &key)
{
    m_pp.removeMacro(key);
}

BuiltInLocationMap
Pipeline::vertexShaderInputLocations() const
{
    return m_vsInputLocations;
}

void
Pipeline::setVertexShaderInputLocations(const BuiltInLocationMap &value)
{
    m_vsInputLocations = value;
}

BuiltInVariableMap
Pipeline::vertexShaderInputVariables() const
{
    return m_vsInputVariables;
}

void
Pipeline::setVertexShaderInputVariables(const BuiltInVariableMap &value)
{
    m_vsInputVariables = value;
}

BuiltInVariableMap
Pipeline::pixelShaderInputVariables() const
{
    return m_psInputVariables;
}

void
Pipeline::setPixelShaderInputVariables(const BuiltInVariableMap &value)
{
    m_psInputVariables = value;
}

std::string
Pipeline::metalShaderEntryPoint() const
{
    return m_metalEntry;
}

void
Pipeline::setMetalShaderEntryPoint(const std::string &value)
{
    m_metalEntry = value;
}

std::string
Pipeline::metalShaderUniformBufferName() const
{
    return m_metalUbo;
}

void
Pipeline::setMetalShaderUniformBufferName(const std::string &value)
{
    m_metalUbo = value;
}

LanguageType
Pipeline::targetLanguage() const
{
    return m_language;
}

void
Pipeline::setTargetLanguage(LanguageType value)
{
    m_language = value;
}

int
Pipeline::version() const
{
    return m_version;
}

void
Pipeline::setVersion(int value)
{
    m_version = value;
}

bool
Pipeline::isOptimizeEnabled() const
{
    return m_optimize;
}

void
Pipeline::setOptimizeEnabled(bool value)
{
    m_optimize = value;
}

bool
Pipeline::isValidationEnabled() const
{
    return m_validate;
}

void
Pipeline::setValidationEnabled(bool value)
{
    m_validate = value;
}

} /* namespace fx9next */

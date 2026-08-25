/*
   Copyright (c) 2015-2023 hkrn All rights reserved

   This file is licensed under MIT license. for more details, see LICENSE.txt.
 */

#pragma once
#ifndef FX9NEXT_PRODUCT_H_
#define FX9NEXT_PRODUCT_H_

#include <stdint.h>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace fx9next {

enum LanguageType {
    kLanguageTypeFirstEnum,
    kLanguageTypeGLSL,
    kLanguageTypeESSL,
    kLanguageTypeHLSL,
    kLanguageTypeMSL,
    kLanguageTypeSPIRV,
    kLanguageTypeMaxEnum
};

struct EffectProduct {
    struct LogSink {
        using StringSet = std::unordered_set<std::string>;
        std::string info;
        std::string debug;
        std::string builder;
        std::string validator;
        StringSet translator;
        StringSet optimizer;
        bool
        isEmpty() const
        {
            return info.empty() && debug.empty() && builder.empty() && translator.empty() && optimizer.empty() &&
                validator.empty();
        }
    } sink;
    std::vector<uint8_t> message;
    size_t numPasses = 0;
    size_t numCompiledPasses = 0;
    size_t numValidatedPasses = 0;
    bool
    hasAllPassCompiled() const
    {
        return numPasses > 0 && numPasses == numCompiledPasses;
    }
    bool
    hasAnyCompiledPass() const
    {
        return numPasses > 0 && numCompiledPasses > 0;
    }
    bool
    isEmpty() const
    {
        return numPasses == 0;
    }
};

typedef std::unordered_map<uint32_t, uint32_t> BuiltInLocationMap;
typedef std::unordered_map<uint32_t, std::string> BuiltInVariableMap;

} /* namespace fx9next */

#endif /* FX9NEXT_PRODUCT_H_ */

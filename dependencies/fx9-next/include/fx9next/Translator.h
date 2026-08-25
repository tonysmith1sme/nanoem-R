/*
   Copyright (c) 2015-2023 hkrn All rights reserved

   This file is licensed under MIT license. for more details, see LICENSE.txt.
 */

#pragma once
#ifndef FX9NEXT_TRANSLATOR_H_
#define FX9NEXT_TRANSLATOR_H_

#include <stdint.h>
#include <string>
#include <unordered_map>
#include <vector>

#include "fx9next/Product.h"

namespace fx9next {

struct TranslateOptions {
    LanguageType language;
    int version;
    std::string metalEntry;
    std::string metalUbo;
};

bool translateSPIRV(const std::vector<uint32_t> &vertex, const std::vector<uint32_t> &fragment,
    const TranslateOptions &options, std::string &vertexSource, std::string &fragmentSource,
    std::vector<uint32_t> &vertexOut, std::vector<uint32_t> &fragmentOut, std::string &error);

} /* namespace fx9next */

#endif /* FX9NEXT_TRANSLATOR_H_ */

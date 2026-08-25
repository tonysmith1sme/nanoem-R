/*
   Copyright (c) 2015-2023 hkrn All rights reserved

   This file is licensed under MIT license. for more details, see LICENSE.txt.
 */

#pragma once
#ifndef FX9NEXT_PRODUCT_WRITER_H_
#define FX9NEXT_PRODUCT_WRITER_H_

#include "fx9next/AST.h"
#include "fx9next/Product.h"

namespace fx9next {

bool writeEffectProduct(const TranslationUnit &unit, LanguageType language, const std::string &metalEntry,
    const std::string &metalUbo, int version, bool validate, EffectProduct &product);

} /* namespace fx9next */

#endif /* FX9NEXT_PRODUCT_WRITER_H_ */

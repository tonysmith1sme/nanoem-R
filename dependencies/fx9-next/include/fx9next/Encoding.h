/*
   Copyright (c) 2015-2023 hkrn All rights reserved

   This file is licensed under MIT license. for more details, see LICENSE.txt.
 */

#pragma once
#ifndef FX9NEXT_ENCODING_H_
#define FX9NEXT_ENCODING_H_

#include <string>

namespace fx9next {

std::string decodeTextSource(const void *bytes, size_t size);
bool isLikelyUtf8(const char *bytes, size_t size);

} /* namespace fx9next */

#endif /* FX9NEXT_ENCODING_H_ */

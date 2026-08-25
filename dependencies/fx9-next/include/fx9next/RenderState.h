/*
   Copyright (c) 2015-2023 hkrn All rights reserved

   This file is licensed under MIT license. for more details, see LICENSE.txt.
 */

#pragma once
#ifndef FX9NEXT_RENDER_STATE_H_
#define FX9NEXT_RENDER_STATE_H_

#include <stdint.h>
#include <string>

namespace fx9next {

bool lookupRenderStateKey(const std::string &name, uint32_t &key);
bool lookupRenderStateValue(const std::string &stateName, const std::string &value, uint32_t &out);

} /* namespace fx9next */

#endif /* FX9NEXT_RENDER_STATE_H_ */

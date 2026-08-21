/*
   Copyright (c) 2015-2023 hkrn All rights reserved

   This file is licensed under MIT license. for more details, see LICENSE.txt.
 */

#ifndef FX9_TEST_COMMON_H_
#define FX9_TEST_COMMON_H_

#include "fx9/Compiler.h"

#include <memory>
#include <sstream>
#include <string>

namespace fx9 {

inline void
compileAllPassesEffect(const std::string &path)
{
    Compiler::initialize();
    Compiler::EffectProduct product;
    bool compiled = false;
    {
        std::unique_ptr<Compiler> compiler(new Compiler(ECoreProfile, EShMsgDefault));
        compiled = compiler->compile(path.c_str(), product);
    }
    Compiler::terminate();
    std::stringstream stream;
    stream << "compiling " << path << "\n"
           << "[info]\n" << product.sink.info << "\n"
           << "[validator]\n" << product.sink.validator << "\n";
    for (auto it = product.sink.translator.begin(), end = product.sink.translator.end(); it != end; ++it) {
        stream << "[translator] " << *it << "\n";
    }
    INFO(stream.str());
    REQUIRE(compiled);
    REQUIRE(product.hasAllPassCompiled());
    REQUIRE_FALSE(product.message.empty());
}

} /* namespace fx9 */

#endif /* FX9_TEST_COMMON_H_ */

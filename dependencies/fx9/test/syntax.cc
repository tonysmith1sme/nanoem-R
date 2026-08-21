/*
   Copyright (c) 2015-2023 hkrn All rights reserved

   This file is licensed under MIT license. for more details, see LICENSE.txt.
 */

#define CATCH_CONFIG_FAST_COMPILE
#include <catch2/catch.hpp>

#include "fx9_test_common.h"

#include <string>
#include <vector>

using namespace fx9;

TEST_CASE("syntax")
{
    static const std::vector<std::string> kEffectNames = {
        "add",
        "and",
        "div",
        "for",
        "function",
        "ge",
        "global_variables",
        "gt",
        "if",
        "le",
        "lt",
        "minus",
        "mul",
        "or",
        "stray_backslash",
        "struct",
    };
    const std::string name = GENERATE(from_range(kEffectNames));
    compileAllPassesEffect(std::string(FX9_TEST_EFFECT_FIXTURES_PATH) + "/syntax/" + name + ".fx");
}

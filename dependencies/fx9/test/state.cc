/*
   Copyright (c) 2015-2023 hkrn All rights reserved

   This file is licensed under MIT license. for more details, see LICENSE.txt.
 */

#define CATCH_CONFIG_FAST_COMPILE
#include <catch2/catch.hpp>

#include "fx9/Compiler.h"

#include <memory>
#include <string>

#include "fx9_test_common.h"

using namespace fx9;

TEST_CASE("state")
{
    compileAllPassesEffect(std::string(FX9_TEST_EFFECT_FIXTURES_PATH) + "/state.fx");
}

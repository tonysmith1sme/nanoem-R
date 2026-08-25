/*
   Copyright (c) 2015-2023 hkrn All rights reserved

   This file is licensed under MIT license. for more details, see LICENSE.txt.
 */

#define CATCH_CONFIG_FAST_COMPILE
#include <catch2/catch.hpp>

#include "fx9next/Preprocessor.h"

using namespace fx9next;

TEST_CASE("fx9next preprocessor expands object macros")
{
    Preprocessor pp;
    pp.setMacro("NANOEM", "1");
    std::string out, err;
    REQUIRE(pp.process("#if NANOEM\nint x;\n#endif\n", "t.fx", out, err));
    REQUIRE(err.empty());
    REQUIRE(out.find("int x;") != std::string::npos);
}

TEST_CASE("fx9next preprocessor token paste")
{
    Preprocessor pp;
    std::string out, err;
    REQUIRE(pp.process("#define JOIN(a,b) a##b\nint JOIN(foo,bar);\n", "t.fx", out, err));
    REQUIRE(out.find("foobar") != std::string::npos);
}

TEST_CASE("fx9next preprocessor include from map")
{
    Preprocessor pp;
    pp.addIncludeSource("inc.fxsub", "float k = 1;\n");
    std::string out, err;
    REQUIRE(pp.process("#include \"inc.fxsub\"\nfloat y = k;\n", "t.fx", out, err));
    REQUIRE(out.find("float k = 1;") != std::string::npos);
    REQUIRE(out.find("float y = k;") != std::string::npos);
}

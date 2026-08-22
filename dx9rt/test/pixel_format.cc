/*
   Copyright (c) 2015-2023 hkrn All rights reserved

   This file is part of emapp component and it's licensed under Mozilla Public License. see LICENSE.md for more details.
 */

#define CATCH_CONFIG_FAST_COMPILE
#include <catch2/catch.hpp>

#include "dx9rt/PixelFormat.h"

using namespace dx9rt;
using namespace nanoem;

TEST_CASE("pixel-format-from-name")
{
    REQUIRE(pixelFormatFromName("A8R8G8B8").format == SG_PIXELFORMAT_RGBA8);
    REQUIRE(pixelFormatFromName("A8R8G8B8").disposition == kPixelFormatLossy);
    REQUIRE(pixelFormatFromName("A32R32G32B32F").format == SG_PIXELFORMAT_RGBA32F);
    REQUIRE(pixelFormatFromName("A16R16G16B16F").format == SG_PIXELFORMAT_RGBA16F);
    REQUIRE(pixelFormatFromName("G16R16F").format == SG_PIXELFORMAT_RG16F);
    REQUIRE(pixelFormatFromName("R32F").format == SG_PIXELFORMAT_R32F);
    REQUIRE(pixelFormatFromName("A2B10G10R10").format == SG_PIXELFORMAT_RGB10A2);
    REQUIRE(pixelFormatFromName("DXT1").format == SG_PIXELFORMAT_BC1_RGBA);
    REQUIRE(pixelFormatFromName("DXT5").format == SG_PIXELFORMAT_BC3_RGBA);
    REQUIRE(pixelFormatFromName("D24S8").format == SG_PIXELFORMAT_DEPTH_STENCIL);
    REQUIRE(pixelFormatFromName("D16").format == SG_PIXELFORMAT_DEPTH);
    /* exact vs lossy are labeled */
    REQUIRE(pixelFormatFromName("A8").disposition == kPixelFormatExact);
    REQUIRE(pixelFormatFromName("R5G6B5").disposition == kPixelFormatLossy);
    REQUIRE(pixelFormatFromName("X8R8G8B8").disposition == kPixelFormatLossy);
    /* unknown names reported instead of silently mapping */
    REQUIRE(pixelFormatFromName("NoSuchFormat").disposition == kPixelFormatUnknown);
    REQUIRE_FALSE(pixelFormatFromName("NoSuchFormat").isValid());
    REQUIRE(pixelFormatFromName("").disposition == kPixelFormatUnknown);
}

TEST_CASE("pixel-format-srgb-swap")
{
    REQUIRE(convertSRGBPixelFormat(true, SG_PIXELFORMAT_RGBA8) == SG_PIXELFORMAT_SRGB8A8);
    REQUIRE(convertSRGBPixelFormat(false, SG_PIXELFORMAT_SRGB8A8) == SG_PIXELFORMAT_RGBA8);
    /* non swappable formats pass through */
    REQUIRE(convertSRGBPixelFormat(true, SG_PIXELFORMAT_RGBA16F) == SG_PIXELFORMAT_RGBA16F);
    REQUIRE(convertSRGBPixelFormat(false, SG_PIXELFORMAT_R32F) == SG_PIXELFORMAT_R32F);
}

TEST_CASE("pixel-format-table-complete")
{
    /* every entry yields a valid disposition */
    static const char *kNames[] = { "DXT1", "DXT3", "DXT5", "A1", "A8", "L8", "L16", "R16F", "R32F", "A8L8",
        "G16R16", "G16R16F", "G32R32F", "X8R8G8B8", "X8B8G8R8", "A8R8G8B8", "A8B8G8R8", "A16R16G16B16",
        "A16B16G16R16", "A16R16G16B16F", "A16B16G16R16F", "A32R32G32B32F", "A32B32G32R32F", "R5G6B5", "A4R4G4B4",
        "A1R5G5B5", "A2B10G10R10", "D16", "D24X8", "D24S8", "D32", "DF16", "DF24", "D32F_LOCKABLE", "S8_LOCKABLE" };
    for (size_t i = 0; i < sizeof(kNames) / sizeof(kNames[0]); i++) {
        const PixelFormatResult result = pixelFormatFromName(kNames[i]);
        INFO("name: " << kNames[i]);
        REQUIRE(result.isValid());
    }
}

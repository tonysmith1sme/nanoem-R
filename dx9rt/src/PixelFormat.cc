/*
   Copyright (c) 2015-2023 hkrn All rights reserved

   This file is part of emapp component and it's licensed under Mozilla Public License. see LICENSE.md for more details.
 */

#include "dx9rt/PixelFormat.h"

#include <string.h>

namespace dx9rt {

using namespace nanoem;

namespace {

struct NamedFormat {
    const char *name;
    sg_pixel_format format;
    PixelFormatDisposition disposition;
};

const NamedFormat kNamedFormats[] = {
    { "DXT1", SG_PIXELFORMAT_BC1_RGBA, kPixelFormatExact },
    { "DXT3", SG_PIXELFORMAT_BC2_RGBA, kPixelFormatExact },
    { "DXT5", SG_PIXELFORMAT_BC3_RGBA, kPixelFormatExact },
    { "A1", SG_PIXELFORMAT_R8, kPixelFormatLossy },
    { "A8", SG_PIXELFORMAT_R8, kPixelFormatExact },
    { "L8", SG_PIXELFORMAT_R8, kPixelFormatLossy },
    { "L16", SG_PIXELFORMAT_R16, kPixelFormatLossy },
    { "R16F", SG_PIXELFORMAT_R16F, kPixelFormatExact },
    { "R32F", SG_PIXELFORMAT_R32F, kPixelFormatExact },
    { "A8L8", SG_PIXELFORMAT_RG8, kPixelFormatLossy },
    { "G16R16", SG_PIXELFORMAT_RG16, kPixelFormatExact },
    { "G16R16F", SG_PIXELFORMAT_RG16F, kPixelFormatExact },
    { "G32R32F", SG_PIXELFORMAT_RG32F, kPixelFormatExact },
    { "X8R8G8B8", SG_PIXELFORMAT_RGBA8, kPixelFormatLossy },
    { "X8B8G8R8", SG_PIXELFORMAT_RGBA8, kPixelFormatLossy },
    { "A8R8G8B8", SG_PIXELFORMAT_RGBA8, kPixelFormatLossy },
    { "A8B8G8R8", SG_PIXELFORMAT_RGBA8, kPixelFormatLossy },
    { "A16R16G16B16", SG_PIXELFORMAT_RGBA16, kPixelFormatLossy },
    { "A16B16G16R16", SG_PIXELFORMAT_RGBA16, kPixelFormatLossy },
    { "A16R16G16B16F", SG_PIXELFORMAT_RGBA16F, kPixelFormatLossy },
    { "A16B16G16R16F", SG_PIXELFORMAT_RGBA16F, kPixelFormatLossy },
    { "A32R32G32B32F", SG_PIXELFORMAT_RGBA32F, kPixelFormatLossy },
    { "A32B32G32R32F", SG_PIXELFORMAT_RGBA32F, kPixelFormatLossy },
    { "R5G6B5", SG_PIXELFORMAT_RGBA8, kPixelFormatLossy },
    { "A4R4G4B4", SG_PIXELFORMAT_RGBA8, kPixelFormatLossy },
    { "A1R5G5B5", SG_PIXELFORMAT_RGBA8, kPixelFormatLossy },
    { "A2B10G10R10", SG_PIXELFORMAT_RGB10A2, kPixelFormatLossy },
    { "D16", SG_PIXELFORMAT_DEPTH, kPixelFormatExact },
    { "D24X8", SG_PIXELFORMAT_DEPTH, kPixelFormatExact },
    { "D24S8", SG_PIXELFORMAT_DEPTH_STENCIL, kPixelFormatExact },
    { "D32", SG_PIXELFORMAT_DEPTH, kPixelFormatExact },
    { "DF16", SG_PIXELFORMAT_DEPTH, kPixelFormatExact },
    { "DF24", SG_PIXELFORMAT_DEPTH, kPixelFormatExact },
    { "D32F_LOCKABLE", SG_PIXELFORMAT_DEPTH, kPixelFormatExact },
    { "S8_LOCKABLE", SG_PIXELFORMAT_DEPTH_STENCIL, kPixelFormatExact },
};

} /* namespace anonymous */

PixelFormatResult
pixelFormatFromName(const char *name)
{
    for (size_t i = 0; i < sizeof(kNamedFormats) / sizeof(kNamedFormats[0]); i++) {
        if (strcmp(kNamedFormats[i].name, name) == 0) {
            return PixelFormatResult(kNamedFormats[i].format, kNamedFormats[i].disposition);
        }
    }
    return PixelFormatResult();
}

sg_pixel_format
convertSRGBPixelFormat(bool enabled, sg_pixel_format format)
{
    if (enabled && format == SG_PIXELFORMAT_RGBA8) {
        return SG_PIXELFORMAT_SRGB8A8;
    }
    if (!enabled && format == SG_PIXELFORMAT_SRGB8A8) {
        return SG_PIXELFORMAT_RGBA8;
    }
    return format;
}

} /* namespace dx9rt */

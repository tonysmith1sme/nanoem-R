/*
   Copyright (c) 2015-2023 hkrn All rights reserved

   This file is part of emapp component and it's licensed under Mozilla Public License. see LICENSE.md for more details.
 */

/// D3DFMT name -> sokol pixel format conversion, mirroring the table historically
/// registered in emapp's Effect constructor. Lossy conversions (16-bit RGB originals
/// collapsing to RGBA8, channel-order differences) are reported instead of being silent.

#ifndef DX9RT_PIXELFORMAT_H_
#define DX9RT_PIXELFORMAT_H_

#include "dx9rt/Types.h"

#include "emapp/Forward.h"

namespace dx9rt {

enum PixelFormatDisposition {
    kPixelFormatExact = 0,
    kPixelFormatLossy, /* precision or channel order differs from the D3D original */
    kPixelFormatUnknown,
};

struct PixelFormatResult {
    nanoem::sg_pixel_format format;
    PixelFormatDisposition disposition;

    PixelFormatResult()
        : format(nanoem::_SG_PIXELFORMAT_DEFAULT)
        , disposition(kPixelFormatUnknown)
    {
    }
    PixelFormatResult(nanoem::sg_pixel_format value, PixelFormatDisposition disposition_)
        : format(value)
        , disposition(disposition_)
    {
    }
    bool isValid() const
    {
        return disposition != kPixelFormatUnknown;
    }
};

/// Resolve a D3DFMT-style format name (as written in MME "Format=" annotations).
PixelFormatResult pixelFormatFromName(const char *name);

/// Swap between linear and sRGB variants for D3DSAMP_SRGBTEXTURE / D3DRS_SRGBWRITEENABLE.
nanoem::sg_pixel_format convertSRGBPixelFormat(bool enabled, nanoem::sg_pixel_format format);

} /* namespace dx9rt */

#endif /* DX9RT_PIXELFORMAT_H_ */

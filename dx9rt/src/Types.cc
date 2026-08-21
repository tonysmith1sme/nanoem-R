/*
   Copyright (c) 2015-2023 hkrn All rights reserved

   This file is part of emapp component and it's licensed under Mozilla Public License. see LICENSE.md for more details.
 */

#include "dx9rt/Types.h"

#include <cstring>

namespace dx9rt {

namespace {

struct RenderStateTableEntry {
    uint32_t key;
    uint32_t defaultValue;
};

/// Compact-index-ordered key and D3D9 documented default pairs.
/// WRAP0..WRAP15 is stored once (key 128 + n) to keep the table flat.
const RenderStateTableEntry kRenderStateTable[kRenderStateIndexWrap0] = {
    { kRenderStateZEnable, kZBufferTrue },
    { kRenderStateFillMode, kFillModeSolid },
    { kRenderStateShadeMode, kShadeModeGouraud },
    { kRenderStateZWriteEnable, 1 },
    { kRenderStateAlphaTestEnable, 0 },
    { kRenderStateLastPixel, 1 },
    { kRenderStateSrcBlend, kBlendOne },
    { kRenderStateDestBlend, kBlendZero },
    { kRenderStateCullMode, kCullModeCCW },
    { kRenderStateZFunc, kCompareFuncLessEqual },
    { kRenderStateAlphaRef, 0 },
    { kRenderStateAlphaFunc, kCompareFuncAlways },
    { kRenderStateDitherEnable, 0 },
    { kRenderStateAlphaBlendEnable, 0 },
    { kRenderStateFogEnable, 0 },
    { kRenderStateSpecularEnable, 0 },
    { kRenderStateFogColor, 0x00000000 },
    { kRenderStateFogTableMode, kFogModeNone },
    { kRenderStateFogStart, encodeFloatBits(0.0f) },
    { kRenderStateFogEnd, encodeFloatBits(1.0f) },
    { kRenderStateFogDensity, encodeFloatBits(1.0f) },
    { kRenderStateRangeFogEnable, 0 },
    { kRenderStateStencilEnable, 0 },
    { kRenderStateStencilFail, kStencilOperatorKeep },
    { kRenderStateStencilZFail, kStencilOperatorKeep },
    { kRenderStateStencilPass, kStencilOperatorKeep },
    { kRenderStateStencilFunc, kCompareFuncAlways },
    { kRenderStateStencilRef, 0 },
    { kRenderStateStencilMask, 0xffffffffu },
    { kRenderStateStencilWriteMask, 0xffffffffu },
    { kRenderStateTextureFactor, 0xffffffffu },
    { kRenderStateClipping, 1 },
    { kRenderStateLighting, 1 },
    { kRenderStateAmbient, 0x00000000 },
    { kRenderStateFogVertexMode, kFogModeNone },
    { kRenderStateColorVertex, 1 },
    { kRenderStateLocalViewer, 1 },
    { kRenderStateNormalizeNormals, 0 },
    { kRenderStateDiffuseMaterialSource, kMaterialColorSourceColor1 },
    { kRenderStateSpecularMaterialSource, kMaterialColorSourceColor2 },
    { kRenderStateAmbientMaterialSource, kMaterialColorSourceMaterial },
    { kRenderStateEmissiveMaterialSource, kMaterialColorSourceMaterial },
    { kRenderStateVertexBlend, 0 },
    { kRenderStateClipPlaneEnable, 0 },
    { kRenderStatePointSize, encodeFloatBits(1.0f) },
    { kRenderStatePointSizeMin, encodeFloatBits(0.0f) },
    { kRenderStatePointSizeMax, encodeFloatBits(64.0f) },
    { kRenderStatePointSpriteEnable, 0 },
    { kRenderStatePointScaleEnable, 0 },
    { kRenderStatePointScaleA, encodeFloatBits(1.0f) },
    { kRenderStatePointScaleB, encodeFloatBits(0.0f) },
    { kRenderStatePointScaleC, encodeFloatBits(0.0f) },
    { kRenderStateMultiSampleAntialias, 1 },
    { kRenderStateMultiSampleMask, 0xffffffffu },
    { kRenderStatePatchEdgeStyle, 0 },
    { kRenderStateDebugMonitorToken, 0 },
    { kRenderStateIndexedVertexBlendEnable, 0 },
    { kRenderStateColorWriteEnable, kColorWriteEnableAll },
    { kRenderStateColorWriteEnable1, kColorWriteEnableAll },
    { kRenderStateColorWriteEnable2, kColorWriteEnableAll },
    { kRenderStateColorWriteEnable3, kColorWriteEnableAll },
    { kRenderStateTweenFactor, encodeFloatBits(0.0f) },
    { kRenderStateBlendOp, kBlendOperatorAdd },
    { kRenderStatePositionDegree, 3 },
    { kRenderStateNormalDegree, 1 },
    { kRenderStateScissorTestEnable, 0 },
    { kRenderStateSlopeScaleDepthBias, encodeFloatBits(0.0f) },
    { kRenderStateAntialiasedLineEnable, 0 },
    { kRenderStateMinTessellationLevel, encodeFloatBits(1.0f) },
    { kRenderStateMaxTessellationLevel, encodeFloatBits(1.0f) },
    { kRenderStateAdaptiveTessX, encodeFloatBits(0.0f) },
    { kRenderStateAdaptiveTessY, encodeFloatBits(0.0f) },
    { kRenderStateAdaptiveTessZ, encodeFloatBits(0.0f) },
    { kRenderStateAdaptiveTessW, encodeFloatBits(0.0f) },
    { kRenderStateEnableAdaptiveTessellation, 0 },
    { kRenderStateTwoSidedStencilMode, 0 },
    { kRenderStateCCWStencilFail, kStencilOperatorKeep },
    { kRenderStateCCWStencilZFail, kStencilOperatorKeep },
    { kRenderStateCCWStencilPass, kStencilOperatorKeep },
    { kRenderStateCCWStencilFunc, kCompareFuncAlways },
    { kRenderStateBlendFactor, 0xffffffffu },
    { kRenderStateSRGBWriteEnable, 0 },
    { kRenderStateDepthBias, encodeFloatBits(0.0f) },
    { kRenderStateSeparateAlphaBlendEnable, 0 },
    { kRenderStateSrcBlendAlpha, kBlendOne },
    { kRenderStateDestBlendAlpha, kBlendZero },
    { kRenderStateBlendOpAlpha, kBlendOperatorAdd },
};

const uint32_t kSamplerStateTable[kSamplerStateIndexMaxEnum] = {
    kSamplerStateAddressU, /* default */
    kSamplerStateAddressV,
    kSamplerStateAddressW,
    kSamplerStateBorderColor,
    kSamplerStateMagFilter,
    kSamplerStateMinFilter,
    kSamplerStateMipFilter,
    kSamplerStateMipMapLODBias,
    kSamplerStateMaxMipLevel,
    kSamplerStateMaxAnisotropy,
    kSamplerStateSRGBTexture,
    kSamplerStateElementIndex,
    kSamplerStateDMAPOffset,
};

const uint32_t kSamplerStateDefaults[kSamplerStateIndexMaxEnum] = {
    kTextureAddressWrap,
    kTextureAddressWrap,
    kTextureAddressWrap,
    0x00000000,
    kTextureFilterPoint,
    kTextureFilterPoint,
    kTextureFilterNone,
    encodeFloatBits(0.0f),
    0,
    1,
    0,
    0,
    0,
};

int
numFlatRenderStates()
{
    return kRenderStateIndexWrap0;
}

} /* namespace anonymous */

int
renderStateIndexFromKey(uint32_t key)
{
    const int numFlat = numFlatRenderStates();
    for (int i = 0; i < numFlat; i++) {
        if (kRenderStateTable[i].key == key) {
            return i;
        }
    }
    /* WRAP0..7 are 128..135 and WRAP8..15 are 198..205 (CLIPPING and others sit between) */
    if (key >= kRenderStateWrap0 && key < kRenderStateWrap0 + 8) {
        return numFlat + int(key - kRenderStateWrap0);
    }
    if (key >= kRenderStateWrap8 && key < kRenderStateWrap8 + 8) {
        return numFlat + 8 + int(key - kRenderStateWrap8);
    }
    return -1;
}

uint32_t
renderStateKeyFromIndex(int index)
{
    const int numFlat = numFlatRenderStates();
    if (index >= 0 && index < numFlat) {
        return kRenderStateTable[index].key;
    }
    if (index >= numFlat && index < numFlat + 8) {
        return kRenderStateWrap0 + uint32_t(index - numFlat);
    }
    if (index >= numFlat + 8 && index < kRenderStateIndexMaxEnum) {
        return kRenderStateWrap8 + uint32_t(index - numFlat - 8);
    }
    return 0;
}

int
samplerStateIndexFromKey(uint32_t key)
{
    for (int i = 0; i < kSamplerStateIndexMaxEnum; i++) {
        if (kSamplerStateTable[i] == key) {
            return i;
        }
    }
    return -1;
}

uint32_t
renderStateDefaultValue(int index)
{
    const int numFlat = numFlatRenderStates();
    if (index >= 0 && index < numFlat) {
        return kRenderStateTable[index].defaultValue;
    }
    if (index >= numFlat && index < kRenderStateIndexMaxEnum) {
        return 0; /* WRAPn default */
    }
    return 0;
}

uint32_t
samplerStateDefaultValue(int index)
{
    if (index >= 0 && index < kSamplerStateIndexMaxEnum) {
        return kSamplerStateDefaults[index];
    }
    return 0;
}

uint32_t
encodeFloatBits(float value)
{
    uint32_t bits;
    memcpy(&bits, &value, sizeof(bits));
    return bits;
}

float
decodeFloatBits(uint32_t value)
{
    float result;
    memcpy(&result, &value, sizeof(result));
    return result;
}

} /* namespace dx9rt */

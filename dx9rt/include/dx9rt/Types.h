/*
   Copyright (c) 2015-2023 hkrn All rights reserved

   This file is part of emapp component and it's licensed under Mozilla Public License. see LICENSE.md for more details.
 */

/// dx9rt - Direct3D 9 semantics runtime for MME effect execution.
///
/// Types.h carries the D3D9 render state / sampler state / pixel format constants with
/// their authoritative numeric values (matching the fx9 protobuf keys emitted from
/// EffectPipeline's m_renderStateEnumConversions) together with the D3D9 default state
/// table documented in the Direct3D 9 D3DRENDERSTATETYPE / D3DSAMPLERSTATETYPE pages.

#ifndef DX9RT_TYPES_H_
#define DX9RT_TYPES_H_

#include <stdint.h>

namespace dx9rt {

/// D3DRENDERSTATETYPE subset reachable from .fx pass states.
/// Numeric values are the real D3D9 values and are kept identical to the keys
/// fx9 emits in Fx9__Effect__RenderState (see EffectPipeline m_renderStateEnumConversions).
enum RenderStateType {
    kRenderStateZEnable = 7,
    kRenderStateFillMode = 8,
    kRenderStateShadeMode = 9,
    kRenderStateZWriteEnable = 14,
    kRenderStateAlphaTestEnable = 15,
    kRenderStateLastPixel = 16,
    kRenderStateSrcBlend = 19,
    kRenderStateDestBlend = 20,
    kRenderStateCullMode = 22,
    kRenderStateZFunc = 23,
    kRenderStateAlphaRef = 24,
    kRenderStateAlphaFunc = 25,
    kRenderStateDitherEnable = 26,
    kRenderStateAlphaBlendEnable = 27,
    kRenderStateFogEnable = 28,
    kRenderStateSpecularEnable = 29,
    kRenderStateFogColor = 34,
    kRenderStateFogTableMode = 35,
    kRenderStateFogStart = 36,
    kRenderStateFogEnd = 37,
    kRenderStateFogDensity = 38,
    kRenderStateRangeFogEnable = 48,
    kRenderStateStencilEnable = 52,
    kRenderStateStencilFail = 53,
    kRenderStateStencilZFail = 54,
    kRenderStateStencilPass = 55,
    kRenderStateStencilFunc = 56,
    kRenderStateStencilRef = 57,
    kRenderStateStencilMask = 58,
    kRenderStateStencilWriteMask = 59,
    kRenderStateTextureFactor = 60,
    kRenderStateWrap0 = 128,
    kRenderStateWrap1 = 129,
    kRenderStateWrap2 = 130,
    kRenderStateWrap3 = 131,
    kRenderStateWrap4 = 132,
    kRenderStateWrap5 = 133,
    kRenderStateWrap6 = 134,
    kRenderStateWrap7 = 135,
    kRenderStateClipping = 136,
    kRenderStateLighting = 137,
    kRenderStateAmbient = 139,
    kRenderStateFogVertexMode = 140,
    kRenderStateColorVertex = 141,
    kRenderStateLocalViewer = 142,
    kRenderStateNormalizeNormals = 143,
    kRenderStateDiffuseMaterialSource = 145,
    kRenderStateSpecularMaterialSource = 146,
    kRenderStateAmbientMaterialSource = 147,
    kRenderStateEmissiveMaterialSource = 148,
    kRenderStateVertexBlend = 151,
    kRenderStateClipPlaneEnable = 152,
    kRenderStatePointSize = 154,
    kRenderStatePointSizeMin = 155,
    kRenderStatePointSizeMax = 166,
    kRenderStatePointSpriteEnable = 156,
    kRenderStatePointScaleEnable = 157,
    kRenderStatePointScaleA = 158,
    kRenderStatePointScaleB = 159,
    kRenderStatePointScaleC = 160,
    kRenderStateMultiSampleAntialias = 161,
    kRenderStateMultiSampleMask = 162,
    kRenderStatePatchEdgeStyle = 163,
    kRenderStateDebugMonitorToken = 165,
    kRenderStateIndexedVertexBlendEnable = 167,
    kRenderStateColorWriteEnable = 168,
    kRenderStateColorWriteEnable1 = 190,
    kRenderStateColorWriteEnable2 = 191,
    kRenderStateColorWriteEnable3 = 192,
    kRenderStateTweenFactor = 170,
    kRenderStateBlendOp = 171,
    kRenderStatePositionDegree = 172,
    kRenderStateNormalDegree = 173,
    kRenderStateScissorTestEnable = 174,
    kRenderStateSlopeScaleDepthBias = 175,
    kRenderStateAntialiasedLineEnable = 176,
    kRenderStateMinTessellationLevel = 178,
    kRenderStateMaxTessellationLevel = 179,
    kRenderStateAdaptiveTessX = 180,
    kRenderStateAdaptiveTessY = 181,
    kRenderStateAdaptiveTessZ = 182,
    kRenderStateAdaptiveTessW = 183,
    kRenderStateEnableAdaptiveTessellation = 184,
    kRenderStateTwoSidedStencilMode = 185,
    kRenderStateCCWStencilFail = 186,
    kRenderStateCCWStencilZFail = 187,
    kRenderStateCCWStencilPass = 188,
    kRenderStateCCWStencilFunc = 189,
    kRenderStateBlendFactor = 193,
    kRenderStateSRGBWriteEnable = 194,
    kRenderStateDepthBias = 195,
    kRenderStateWrap8 = 198,
    kRenderStateWrap9 = 199,
    kRenderStateWrap10 = 200,
    kRenderStateWrap11 = 201,
    kRenderStateWrap12 = 202,
    kRenderStateWrap13 = 203,
    kRenderStateWrap14 = 204,
    kRenderStateWrap15 = 205,
    kRenderStateSeparateAlphaBlendEnable = 206,
    kRenderStateSrcBlendAlpha = 207,
    kRenderStateDestBlendAlpha = 208,
    kRenderStateBlendOpAlpha = 209,
};

/// D3DSAMPLERSTATETYPE.
enum SamplerStateType {
    kSamplerStateAddressU = 1,
    kSamplerStateAddressV = 2,
    kSamplerStateAddressW = 3,
    kSamplerStateBorderColor = 4,
    kSamplerStateMagFilter = 5,
    kSamplerStateMinFilter = 6,
    kSamplerStateMipFilter = 7,
    kSamplerStateMipMapLODBias = 8,
    kSamplerStateMaxMipLevel = 9,
    kSamplerStateMaxAnisotropy = 10,
    kSamplerStateSRGBTexture = 11,
    kSamplerStateElementIndex = 12,
    kSamplerStateDMAPOffset = 13,
};

/// D3DBLEND values (D3DRS_SRCBLEND and friends).
enum BlendType {
    kBlendZero = 1,
    kBlendOne = 2,
    kBlendSrcColor = 3,
    kBlendInvSrcColor = 4,
    kBlendSrcAlpha = 5,
    kBlendInvSrcAlpha = 6,
    kBlendDestAlpha = 7,
    kBlendInvDestAlpha = 8,
    kBlendDestColor = 9,
    kBlendInvDestColor = 10,
    kBlendSrcAlphaSat = 11,
    kBlendBothSrcAlpha = 12,
    kBlendBothInvSrcAlpha = 13,
    kBlendBlendFactor = 14,
    kBlendInvBlendFactor = 15,
};

/// D3DBLENDOP values.
enum BlendOperatorType {
    kBlendOperatorAdd = 1,
    kBlendOperatorSubtract = 2,
    kBlendOperatorRevSubtract = 3,
    kBlendOperatorMin = 4,
    kBlendOperatorMax = 5,
};

/// D3DCMP values.
enum CompareFuncType {
    kCompareFuncNever = 1,
    kCompareFuncLess = 2,
    kCompareFuncEqual = 3,
    kCompareFuncLessEqual = 4,
    kCompareFuncGreater = 5,
    kCompareFuncNotEqual = 6,
    kCompareFuncGreaterEqual = 7,
    kCompareFuncAlways = 8,
};

/// D3DSTENCILOP values.
enum StencilOperatorType {
    kStencilOperatorKeep = 1,
    kStencilOperatorZero = 2,
    kStencilOperatorReplace = 3,
    kStencilOperatorIncrSat = 4,
    kStencilOperatorDecrSat = 5,
    kStencilOperatorInvert = 6,
    kStencilOperatorIncr = 7,
    kStencilOperatorDecr = 8,
};

/// D3DCULL values.
enum CullModeType {
    kCullModeNone = 1,
    kCullModeCW = 2,
    kCullModeCCW = 3,
};

/// D3DFILLMODE values.
enum FillModeType {
    kFillModePoint = 1,
    kFillModeWireFrame = 2,
    kFillModeSolid = 3,
};

/// D3DSHADEMODE values.
enum ShadeModeType {
    kShadeModeFlat = 1,
    kShadeModeGouraud = 2,
    kShadeModePhong = 3,
};

/// D3DFOGMODE values.
enum FogModeType {
    kFogModeNone = 0,
    kFogModeExp = 1,
    kFogModeExp2 = 2,
    kFogModeLinear = 3,
};

/// D3DTEXTUREADDRESS values.
enum TextureAddressType {
    kTextureAddressWrap = 1,
    kTextureAddressMirror = 2,
    kTextureAddressClamp = 3,
    kTextureAddressBorder = 4,
    kTextureAddressMirrorOnce = 5,
};

/// D3DTEXTUREFILTERTYPE values.
enum TextureFilterType {
    kTextureFilterNone = 0,
    kTextureFilterPoint = 1,
    kTextureFilterLinear = 2,
    kTextureFilterAnisotropic = 3,
};

/// D3DZBUFFERTYPE values (D3DRS_ZENABLE).
enum ZBufferType {
    kZBufferFalse = 0,
    kZBufferTrue = 1,
    kZBufferUseW = 2,
};

/// D3DMATERIALCOLORSOURCE values.
enum MaterialColorSourceType {
    kMaterialColorSourceMaterial = 0,
    kMaterialColorSourceColor1 = 1,
    kMaterialColorSourceColor2 = 2,
};

/// Channel masks of D3DRS_COLORWRITEENABLE (D3DCOLORWRITEENABLE_*).
enum ColorWriteMaskType {
    kColorWriteEnableRed = 1,
    kColorWriteEnableGreen = 2,
    kColorWriteEnableBlue = 4,
    kColorWriteEnableAlpha = 8,
    kColorWriteEnableAll = 15,
};

/// Compact index of every known render state, used to lay out StateVector's storage.
/// Order must match kRenderStateDefaults below.
enum RenderStateIndex {
    kRenderStateIndexZEnable = 0,
    kRenderStateIndexFillMode,
    kRenderStateIndexShadeMode,
    kRenderStateIndexZWriteEnable,
    kRenderStateIndexAlphaTestEnable,
    kRenderStateIndexLastPixel,
    kRenderStateIndexSrcBlend,
    kRenderStateIndexDestBlend,
    kRenderStateIndexCullMode,
    kRenderStateIndexZFunc,
    kRenderStateIndexAlphaRef,
    kRenderStateIndexAlphaFunc,
    kRenderStateIndexDitherEnable,
    kRenderStateIndexAlphaBlendEnable,
    kRenderStateIndexFogEnable,
    kRenderStateIndexSpecularEnable,
    kRenderStateIndexFogColor,
    kRenderStateIndexFogTableMode,
    kRenderStateIndexFogStart,
    kRenderStateIndexFogEnd,
    kRenderStateIndexFogDensity,
    kRenderStateIndexRangeFogEnable,
    kRenderStateIndexStencilEnable,
    kRenderStateIndexStencilFail,
    kRenderStateIndexStencilZFail,
    kRenderStateIndexStencilPass,
    kRenderStateIndexStencilFunc,
    kRenderStateIndexStencilRef,
    kRenderStateIndexStencilMask,
    kRenderStateIndexStencilWriteMask,
    kRenderStateIndexTextureFactor,
    kRenderStateIndexClipping,
    kRenderStateIndexLighting,
    kRenderStateIndexAmbient,
    kRenderStateIndexFogVertexMode,
    kRenderStateIndexColorVertex,
    kRenderStateIndexLocalViewer,
    kRenderStateIndexNormalizeNormals,
    kRenderStateIndexDiffuseMaterialSource,
    kRenderStateIndexSpecularMaterialSource,
    kRenderStateIndexAmbientMaterialSource,
    kRenderStateIndexEmissiveMaterialSource,
    kRenderStateIndexVertexBlend,
    kRenderStateIndexClipPlaneEnable,
    kRenderStateIndexPointSize,
    kRenderStateIndexPointSizeMin,
    kRenderStateIndexPointSizeMax,
    kRenderStateIndexPointSpriteEnable,
    kRenderStateIndexPointScaleEnable,
    kRenderStateIndexPointScaleA,
    kRenderStateIndexPointScaleB,
    kRenderStateIndexPointScaleC,
    kRenderStateIndexMultiSampleAntialias,
    kRenderStateIndexMultiSampleMask,
    kRenderStateIndexPatchEdgeStyle,
    kRenderStateIndexDebugMonitorToken,
    kRenderStateIndexIndexedVertexBlendEnable,
    kRenderStateIndexColorWriteEnable,
    kRenderStateIndexColorWriteEnable1,
    kRenderStateIndexColorWriteEnable2,
    kRenderStateIndexColorWriteEnable3,
    kRenderStateIndexTweenFactor,
    kRenderStateIndexBlendOp,
    kRenderStateIndexPositionDegree,
    kRenderStateIndexNormalDegree,
    kRenderStateIndexScissorTestEnable,
    kRenderStateIndexSlopeScaleDepthBias,
    kRenderStateIndexAntialiasedLineEnable,
    kRenderStateIndexMinTessellationLevel,
    kRenderStateIndexMaxTessellationLevel,
    kRenderStateIndexAdaptiveTessX,
    kRenderStateIndexAdaptiveTessY,
    kRenderStateIndexAdaptiveTessZ,
    kRenderStateIndexAdaptiveTessW,
    kRenderStateIndexEnableAdaptiveTessellation,
    kRenderStateIndexTwoSidedStencilMode,
    kRenderStateIndexCCWStencilFail,
    kRenderStateIndexCCWStencilZFail,
    kRenderStateIndexCCWStencilPass,
    kRenderStateIndexCCWStencilFunc,
    kRenderStateIndexBlendFactor,
    kRenderStateIndexSRGBWriteEnable,
    kRenderStateIndexDepthBias,
    kRenderStateIndexSeparateAlphaBlendEnable,
    kRenderStateIndexSrcBlendAlpha,
    kRenderStateIndexDestBlendAlpha,
    kRenderStateIndexBlendOpAlpha,
    kRenderStateIndexWrap0, /* .. Wrap15 are contiguous right after Wrap0 */
    kRenderStateIndexMaxEnum = kRenderStateIndexWrap0 + 16,
};

/// Number of contiguous D3DRS_WRAP0..WRAP15 indices.
static const int kNumWrapStates = 16;

/// Compact index of every known sampler state, used to lay out per-sampler storage.
enum SamplerStateIndex {
    kSamplerStateIndexAddressU = 0,
    kSamplerStateIndexAddressV,
    kSamplerStateIndexAddressW,
    kSamplerStateIndexBorderColor,
    kSamplerStateIndexMagFilter,
    kSamplerStateIndexMinFilter,
    kSamplerStateIndexMipFilter,
    kSamplerStateIndexMipMapLODBias,
    kSamplerStateIndexMaxMipLevel,
    kSamplerStateIndexMaxAnisotropy,
    kSamplerStateIndexSRGBTexture,
    kSamplerStateIndexElementIndex,
    kSamplerStateIndexDMAPOffset,
    kSamplerStateIndexMaxEnum,
};

/// Maximum simultaneous samplers per shader stage (D3D9 has up to 16 texture stages).
static const int kMaxSamplerCount = 16;

/// Map a D3DRS_* numeric key to its compact index; returns -1 when unknown.
int renderStateIndexFromKey(uint32_t key);

/// Reverse of renderStateIndexFromKey.
uint32_t renderStateKeyFromIndex(int index);

/// Map a D3DSAMP_* numeric key to its compact index; returns -1 when unknown.
int samplerStateIndexFromKey(uint32_t key);

/// The documented D3D9 default value of a render state (by compact index).
uint32_t renderStateDefaultValue(int index);

/// The documented D3D9 default value of a sampler state (by compact index).
uint32_t samplerStateDefaultValue(int index);

/// Bit-cast helpers for float-valued states (D3D passes them as DWORD).
uint32_t encodeFloatBits(float value);
float decodeFloatBits(uint32_t value);

} /* namespace dx9rt */

#endif /* DX9RT_TYPES_H_ */

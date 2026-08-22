/*
   Copyright (c) 2015-2023 hkrn All rights reserved

   This file is part of emapp component and it's licensed under Mozilla Public License. see LICENSE.md for more details.
 */

#include "dx9rt/Resolver.h"

namespace dx9rt {

using namespace nanoem;

namespace {

int
sgBlendFactor(uint32_t value)
{
    switch (value) {
    case kBlendZero:
        return SG_BLENDFACTOR_ZERO;
    case kBlendOne:
        return SG_BLENDFACTOR_ONE;
    case kBlendSrcColor:
        return SG_BLENDFACTOR_SRC_COLOR;
    case kBlendInvSrcColor:
        return SG_BLENDFACTOR_ONE_MINUS_SRC_COLOR;
    case kBlendSrcAlpha:
        return SG_BLENDFACTOR_SRC_ALPHA;
    case kBlendInvSrcAlpha:
        return SG_BLENDFACTOR_ONE_MINUS_SRC_ALPHA;
    case kBlendDestAlpha:
        return SG_BLENDFACTOR_DST_ALPHA;
    case kBlendInvDestAlpha:
        return SG_BLENDFACTOR_ONE_MINUS_DST_ALPHA;
    case kBlendDestColor:
        return SG_BLENDFACTOR_DST_COLOR;
    case kBlendInvDestColor:
        return SG_BLENDFACTOR_ONE_MINUS_DST_COLOR;
    case kBlendSrcAlphaSat:
        return SG_BLENDFACTOR_SRC_ALPHA_SATURATED;
    case kBlendBlendFactor:
        return SG_BLENDFACTOR_BLEND_COLOR;
    case kBlendInvBlendFactor:
        return SG_BLENDFACTOR_ONE_MINUS_BLEND_COLOR;
    default:
        return -1;
    }
}

int
sgBlendOperator(uint32_t value)
{
    switch (value) {
    case kBlendOperatorAdd:
        return SG_BLENDOP_ADD;
    case kBlendOperatorSubtract:
        return SG_BLENDOP_SUBTRACT;
    case kBlendOperatorRevSubtract:
        return SG_BLENDOP_REVERSE_SUBTRACT;
#if defined(NANOEM_ENABLE_BLENDOP_MINMAX)
    case kBlendOperatorMin:
        return SG_BLENDOP_MIN;
    case kBlendOperatorMax:
        return SG_BLENDOP_MAX;
#endif
    default:
        return -1;
    }
}

int
sgCompareFunc(uint32_t value)
{
    switch (value) {
    case kCompareFuncNever:
        return SG_COMPAREFUNC_NEVER;
    case kCompareFuncLess:
        return SG_COMPAREFUNC_LESS;
    case kCompareFuncEqual:
        return SG_COMPAREFUNC_EQUAL;
    case kCompareFuncLessEqual:
        return SG_COMPAREFUNC_LESS_EQUAL;
    case kCompareFuncGreater:
        return SG_COMPAREFUNC_GREATER;
    case kCompareFuncNotEqual:
        return SG_COMPAREFUNC_NOT_EQUAL;
    case kCompareFuncGreaterEqual:
        return SG_COMPAREFUNC_GREATER_EQUAL;
    case kCompareFuncAlways:
        return SG_COMPAREFUNC_ALWAYS;
    default:
        return -1;
    }
}

int
sgStencilOperator(uint32_t value)
{
    switch (value) {
    case kStencilOperatorKeep:
        return SG_STENCILOP_KEEP;
    case kStencilOperatorZero:
        return SG_STENCILOP_ZERO;
    case kStencilOperatorReplace:
        return SG_STENCILOP_REPLACE;
    case kStencilOperatorIncrSat:
        return SG_STENCILOP_INCR_CLAMP;
    case kStencilOperatorDecrSat:
        return SG_STENCILOP_DECR_CLAMP;
    case kStencilOperatorInvert:
        return SG_STENCILOP_INVERT;
    case kStencilOperatorIncr:
        return SG_STENCILOP_INCR_WRAP;
    case kStencilOperatorDecr:
        return SG_STENCILOP_DECR_WRAP;
    default:
        return -1;
    }
}

int
sgWrapMode(uint32_t value, bool *approximated)
{
    switch (value) {
    case kTextureAddressWrap:
        return SG_WRAP_REPEAT;
    case kTextureAddressMirror:
        return SG_WRAP_MIRRORED_REPEAT;
    case kTextureAddressClamp:
        return SG_WRAP_CLAMP_TO_EDGE;
    case kTextureAddressBorder:
        return SG_WRAP_CLAMP_TO_BORDER;
    case kTextureAddressMirrorOnce:
        *approximated = true;
        return SG_WRAP_CLAMP_TO_EDGE; /* sokol has no mirror-once */
    default:
        return SG_WRAP_REPEAT;
    }
}

int
sgBorderColor(uint32_t value, bool *approximated)
{
    *approximated = false;
    if (value == 0x00000000) {
        return SG_BORDERCOLOR_TRANSPARENT_BLACK;
    }
    if (value == 0xff000000) {
        return SG_BORDERCOLOR_OPAQUE_BLACK;
    }
    if (value == 0xffffffff) {
        return SG_BORDERCOLOR_OPAQUE_WHITE;
    }
    /* sokol only exposes three presets; approximate by chroma presence */
    *approximated = true;
    return (value & 0x00ffffff) != 0 ? SG_BORDERCOLOR_OPAQUE_WHITE
                                     : ((value & 0xff000000) != 0 ? SG_BORDERCOLOR_OPAQUE_BLACK
                                                                  : SG_BORDERCOLOR_TRANSPARENT_BLACK);
}

} /* namespace anonymous */

ResolvedExtraStates::ResolvedExtraStates()
    : alphaTestEnabled(false)
    , alphaTestReference(0)
    , alphaTestCompareFunc(kCompareFuncAlways)
    , srgbWriteEnabled(false)
    , scissorTestEnabled(false)
{
}

void
resolvePipeline(const StateVector &states, sg_pipeline_desc &desc, ResolvedExtraStates &extra,
    ResolveDiagnostics *diagnostics)
{
    auto note = [diagnostics](uint32_t key, DispositionType disposition, const char *text) {
        if (diagnostics) {
            diagnostics->add(key, disposition, text);
        }
    };
    /* depth: D3D9 defaults ZENABLE=TRUE, ZWRITEENABLE=TRUE, ZFUNC=LESSEQUAL */
    const uint32_t zEnable = states.renderState(kRenderStateZEnable);
    const uint32_t zWrite = states.renderState(kRenderStateZWriteEnable);
    const uint32_t zFunc = states.renderState(kRenderStateZFunc);
    desc.depth.write_enabled = zEnable != kZBufferFalse && zWrite != 0;
    if (int converted = sgCompareFunc(zFunc); converted >= 0) {
        desc.depth.compare = static_cast<sg_compare_func>(converted);
    }
    if (zEnable == kZBufferFalse) {
        desc.depth.compare = SG_COMPAREFUNC_ALWAYS;
    }
    else if (zEnable == kZBufferUseW) {
        note(kRenderStateZEnable, kDispositionApproximated, "w-buffering approximated by z comparison");
    }
    if (states.isRenderStateExplicit(kRenderStateZEnable) || states.isRenderStateExplicit(kRenderStateZWriteEnable) ||
        states.isRenderStateExplicit(kRenderStateZFunc)) {
        note(kRenderStateZEnable, kDispositionImplemented, "depth state");
    }
    /* depth bias: D3D passes float bits; units differ between APIs (approximated) */
    const uint32_t depthBias = states.renderState(kRenderStateDepthBias);
    const uint32_t slopeBias = states.renderState(kRenderStateSlopeScaleDepthBias);
    if (depthBias != 0 || slopeBias != 0) {
        desc.depth.bias = decodeFloatBits(depthBias);
        desc.depth.bias_slope_scale = decodeFloatBits(slopeBias);
        note(kRenderStateDepthBias, kDispositionApproximated, "depth bias unit conversion is approximate");
    }
    /* stencil: front always, back mirrors front unless two sided mode is enabled */
    const bool stencilEnabled = states.renderState(kRenderStateStencilEnable) != 0;
    const bool twoSided = states.renderState(kRenderStateTwoSidedStencilMode) != 0;
    desc.stencil.enabled = stencilEnabled;
    if (int v = sgCompareFunc(states.renderState(kRenderStateStencilFunc)); v >= 0) {
        desc.stencil.front.compare = static_cast<sg_compare_func>(v);
    }
    if (int v = sgStencilOperator(states.renderState(kRenderStateStencilFail)); v >= 0) {
        desc.stencil.front.fail_op = static_cast<sg_stencil_op>(v);
    }
    if (int v = sgStencilOperator(states.renderState(kRenderStateStencilZFail)); v >= 0) {
        desc.stencil.front.depth_fail_op = static_cast<sg_stencil_op>(v);
    }
    if (int v = sgStencilOperator(states.renderState(kRenderStateStencilPass)); v >= 0) {
        desc.stencil.front.pass_op = static_cast<sg_stencil_op>(v);
    }
    if (twoSided) {
        if (int v = sgCompareFunc(states.renderState(kRenderStateCCWStencilFunc)); v >= 0) {
            desc.stencil.back.compare = static_cast<sg_compare_func>(v);
        }
        if (int v = sgStencilOperator(states.renderState(kRenderStateCCWStencilFail)); v >= 0) {
            desc.stencil.back.fail_op = static_cast<sg_stencil_op>(v);
        }
        if (int v = sgStencilOperator(states.renderState(kRenderStateCCWStencilZFail)); v >= 0) {
            desc.stencil.back.depth_fail_op = static_cast<sg_stencil_op>(v);
        }
        if (int v = sgStencilOperator(states.renderState(kRenderStateCCWStencilPass)); v >= 0) {
            desc.stencil.back.pass_op = static_cast<sg_stencil_op>(v);
        }
    }
    else {
        desc.stencil.back = desc.stencil.front;
    }
    desc.stencil.ref = uint8_t(states.renderState(kRenderStateStencilRef) & 0xff);
    /* sokol shares read/write masks across both faces */
    desc.stencil.read_mask = uint8_t(states.renderState(kRenderStateStencilMask) & 0xff);
    desc.stencil.write_mask = uint8_t(states.renderState(kRenderStateStencilWriteMask) & 0xff);
    if (twoSided) {
        note(kRenderStateStencilMask, kDispositionApproximated, "two sided stencil shares ref/masks across faces");
    }
    /* blend: D3D9 defaults ALPHABLENDENABLE=FALSE, SRC=ONE, DEST=ZERO, OP=ADD */
    sg_blend_state &blend = desc.colors[0].blend;
    blend.enabled = states.renderState(kRenderStateAlphaBlendEnable) != 0;
    uint32_t src = states.renderState(kRenderStateSrcBlend), dst = states.renderState(kRenderStateDestBlend);
    if (src == kBlendBothSrcAlpha || dst == kBlendBothSrcAlpha) {
        blend.src_factor_rgb = SG_BLENDFACTOR_SRC_ALPHA;
        blend.dst_factor_rgb = SG_BLENDFACTOR_ONE_MINUS_SRC_ALPHA;
        note(kRenderStateSrcBlend, kDispositionImplemented, "BOTHSRCALPHA expands to src/dst pair");
    }
    else if (src == kBlendBothInvSrcAlpha) {
        blend.src_factor_rgb = SG_BLENDFACTOR_ONE_MINUS_SRC_ALPHA;
        blend.dst_factor_rgb = SG_BLENDFACTOR_SRC_ALPHA;
        note(kRenderStateSrcBlend, kDispositionImplemented, "BOTHINVSRCALPHA expands to src/dst pair");
    }
    else {
        if (int v = sgBlendFactor(src); v >= 0) {
            blend.src_factor_rgb = static_cast<sg_blend_factor>(v);
        }
        if (int v = sgBlendFactor(dst); v >= 0) {
            blend.dst_factor_rgb = static_cast<sg_blend_factor>(v);
        }
    }
    if (int v = sgBlendOperator(states.renderState(kRenderStateBlendOp)); v >= 0) {
        blend.op_rgb = static_cast<sg_blend_op>(v);
    }
    else if (states.renderState(kRenderStateBlendOp) == kBlendOperatorMin ||
        states.renderState(kRenderStateBlendOp) == kBlendOperatorMax) {
        note(kRenderStateBlendOp, kDispositionApproximated, "MIN/MAX blend op needs NANOEM_ENABLE_BLENDOP_MINMAX");
    }
    const bool separateAlpha = states.renderState(kRenderStateSeparateAlphaBlendEnable) != 0;
    if (separateAlpha) {
        if (int v = sgBlendFactor(states.renderState(kRenderStateSrcBlendAlpha)); v >= 0) {
            blend.src_factor_alpha = static_cast<sg_blend_factor>(v == SG_BLENDFACTOR_SRC_ALPHA_SATURATED
                    ? SG_BLENDFACTOR_ONE
                    : (v == SG_BLENDFACTOR_BLEND_COLOR ? SG_BLENDFACTOR_BLEND_ALPHA
                                                       : (v == SG_BLENDFACTOR_ONE_MINUS_BLEND_COLOR
                                                              ? SG_BLENDFACTOR_ONE_MINUS_BLEND_ALPHA
                                                              : v)));
        }
        if (int v = sgBlendFactor(states.renderState(kRenderStateDestBlendAlpha)); v >= 0) {
            blend.dst_factor_alpha = static_cast<sg_blend_factor>(v == SG_BLENDFACTOR_BLEND_COLOR
                    ? SG_BLENDFACTOR_BLEND_ALPHA
                    : (v == SG_BLENDFACTOR_ONE_MINUS_BLEND_COLOR ? SG_BLENDFACTOR_ONE_MINUS_BLEND_ALPHA : v));
        }
        if (int v = sgBlendOperator(states.renderState(kRenderStateBlendOpAlpha)); v >= 0) {
            blend.op_alpha = static_cast<sg_blend_op>(v);
        }
    }
    else {
        blend.src_factor_alpha = blend.src_factor_rgb;
        blend.dst_factor_alpha = blend.dst_factor_rgb;
        blend.op_alpha = blend.op_rgb;
    }
    /* blend factor color: D3D packs ARGB */
    const uint32_t blendFactor = states.renderState(kRenderStateBlendFactor);
    if (states.isRenderStateExplicit(kRenderStateBlendFactor)) {
        static const float kScale = 1.0f / 255.0f;
        desc.blend_color.r = ((blendFactor >> 16) & 0xff) * kScale;
        desc.blend_color.g = ((blendFactor >> 8) & 0xff) * kScale;
        desc.blend_color.b = (blendFactor & 0xff) * kScale;
        desc.blend_color.a = ((blendFactor >> 24) & 0xff) * kScale;
    }
    /* cull: DX9 CCW (default) removes back faces; sokol BACK culls clockwise-wound faces of
       DX-convention geometry, matching the legacy conversion table */
    switch (states.renderState(kRenderStateCullMode)) {
    case kCullModeNone:
        desc.cull_mode = SG_CULLMODE_NONE;
        break;
    case kCullModeCW:
        desc.cull_mode = SG_CULLMODE_FRONT;
        break;
    case kCullModeCCW:
    default:
        desc.cull_mode = SG_CULLMODE_BACK;
        break;
    }
    /* fill mode maps to primitive topology (wireframe is approximate) */
    switch (states.renderState(kRenderStateFillMode)) {
    case kFillModePoint:
        desc.primitive_type = SG_PRIMITIVETYPE_POINTS;
        note(kRenderStateFillMode, kDispositionApproximated, "point fill approximated by POINTS topology");
        break;
    case kFillModeWireFrame:
        desc.primitive_type = SG_PRIMITIVETYPE_LINES;
        note(kRenderStateFillMode, kDispositionApproximated, "wireframe fill approximated by LINES topology");
        break;
    default:
        break; /* SOLID keeps the caller-provided triangle topology */
    }
    /* per attachment color write masks (D3D9 default = all channels) */
    const uint32_t writeKeys[SG_MAX_COLOR_ATTACHMENTS] = { kRenderStateColorWriteEnable,
        kRenderStateColorWriteEnable1, kRenderStateColorWriteEnable2, kRenderStateColorWriteEnable3 };
    for (int i = 0; i < SG_MAX_COLOR_ATTACHMENTS; i++) {
        const uint32_t mask = states.renderState(writeKeys[i]);
        if (states.isRenderStateExplicit(writeKeys[i])) {
            desc.colors[i].write_mask =
                mask != 0 ? static_cast<sg_color_mask>(mask & SG_COLORMASK_RGBA) : SG_COLORMASK_NONE;
        }
    }
    /* shader-level states consumed by fx9 AST injection (P2) */
    extra.alphaTestEnabled = states.renderState(kRenderStateAlphaTestEnable) != 0;
    extra.alphaTestReference = uint8_t(states.renderState(kRenderStateAlphaRef) & 0xff);
    if (int v = sgCompareFunc(states.renderState(kRenderStateAlphaFunc)); v >= 0) {
        extra.alphaTestCompareFunc = static_cast<int>(states.renderState(kRenderStateAlphaFunc));
    }
    if (extra.alphaTestEnabled) {
        note(kRenderStateAlphaTestEnable, kDispositionShaderLevel,
            "alpha test baked into pixel shader discard (reference/255 comparison)");
    }
    extra.srgbWriteEnabled = states.renderState(kRenderStateSRGBWriteEnable) != 0;
    if (extra.srgbWriteEnabled) {
        note(kRenderStateSRGBWriteEnable, kDispositionShaderLevel, "sRGB encode baked into pixel shader output");
    }
    /* runtime-level states applied by the renderer at draw time */
    extra.scissorTestEnabled = states.renderState(kRenderStateScissorTestEnable) != 0;
    if (extra.scissorTestEnabled) {
        note(kRenderStateScissorTestEnable, kDispositionRuntimeLevel, "scissor rect applied at draw time");
    }
    /* explicitly ignored FFP-only states without MME-visible effect */
    static const uint32_t kIgnoredStates[] = {
        kRenderStateShadeMode, /* Gouraud default, ps_3_0 always interpolates */
        kRenderStateLastPixel,
        kRenderStateDitherEnable,
        kRenderStateMultiSampleAntialias,
        kRenderStateMultiSampleMask,
        kRenderStateAntialiasedLineEnable,
        kRenderStateDebugMonitorToken,
        kRenderStatePatchEdgeStyle,
        kRenderStatePositionDegree,
        kRenderStateNormalDegree,
    };
    for (size_t i = 0; i < sizeof(kIgnoredStates) / sizeof(kIgnoredStates[0]); i++) {
        if (states.isRenderStateExplicit(kIgnoredStates[i])) {
            note(kIgnoredStates[i], kDispositionIgnored, "fixed-function only, no MME-visible effect");
        }
    }
    /* FFP lighting/fog/material/point states are recorded approximated; the FFP tier (P4)
       generates shader code only for the combos the corpus reports */
    static const uint32_t kFfpStates[] = {
        kRenderStateFogEnable,
        kRenderStateFogColor,
        kRenderStateFogTableMode,
        kRenderStateFogStart,
        kRenderStateFogEnd,
        kRenderStateFogDensity,
        kRenderStateRangeFogEnable,
        kRenderStateFogVertexMode,
        kRenderStateLighting,
        kRenderStateAmbient,
        kRenderStateColorVertex,
        kRenderStateLocalViewer,
        kRenderStateNormalizeNormals,
        kRenderStateDiffuseMaterialSource,
        kRenderStateSpecularMaterialSource,
        kRenderStateAmbientMaterialSource,
        kRenderStateEmissiveMaterialSource,
        kRenderStateSpecularEnable,
        kRenderStatePointSize,
        kRenderStatePointSizeMin,
        kRenderStatePointSizeMax,
        kRenderStatePointSpriteEnable,
        kRenderStatePointScaleEnable,
        kRenderStatePointScaleA,
        kRenderStatePointScaleB,
        kRenderStatePointScaleC,
        kRenderStateClipping,
        kRenderStateClipPlaneEnable,
        kRenderStateTextureFactor,
    };
    for (size_t i = 0; i < sizeof(kFfpStates) / sizeof(kFfpStates[0]); i++) {
        if (states.isRenderStateExplicit(kFfpStates[i])) {
            note(kFfpStates[i], kDispositionShaderLevel, "fixed-function state, deferred to FFP shader tier");
        }
    }
    /* wrap states (D3DWRAPCOORD u/v/w bits) are legacy T&L texture wrapping */
    for (int i = 0; i < kNumWrapStates; i++) {
        const uint32_t key = renderStateKeyFromIndex(kRenderStateIndexWrap0 + i);
        if (states.isRenderStateExplicit(key)) {
            note(key, kDispositionIgnored, "legacy per-stage coordinate wrap flags");
        }
    }
}

void
resolveSamplerImage(const StateVector &states, int stage, sg_image_desc &desc, ResolveDiagnostics *diagnostics)
{
    if (stage < 0 || stage >= kMaxSamplerCount) {
        return;
    }
    auto note = [diagnostics](uint32_t key, DispositionType disposition, const char *text) {
        if (diagnostics) {
            diagnostics->add(key, disposition, text);
        }
    };
    bool approximated = false;
    desc.wrap_u = static_cast<sg_wrap>(sgWrapMode(states.samplerState(stage, kSamplerStateAddressU), &approximated));
    desc.wrap_v = static_cast<sg_wrap>(sgWrapMode(states.samplerState(stage, kSamplerStateAddressV), &approximated));
    desc.wrap_w = static_cast<sg_wrap>(sgWrapMode(states.samplerState(stage, kSamplerStateAddressW), &approximated));
    if (approximated) {
        note(kSamplerStateAddressU, kDispositionApproximated, "MIRRORONCE approximated by CLAMP_TO_EDGE");
    }
    const uint32_t borderColor = states.samplerState(stage, kSamplerStateBorderColor);
    bool borderApproximated = false;
    desc.border_color = static_cast<sg_border_color>(sgBorderColor(borderColor, &borderApproximated));
    if (borderApproximated) {
        note(kSamplerStateBorderColor, kDispositionApproximated, "border color approximated to nearest preset");
    }
    const uint32_t magFilter = states.samplerState(stage, kSamplerStateMagFilter);
    desc.mag_filter = magFilter >= kTextureFilterLinear ? SG_FILTER_LINEAR : SG_FILTER_NEAREST;
    if (magFilter == kTextureFilterAnisotropic) {
        desc.max_anisotropy = 16;
    }
    const uint32_t minFilter = states.samplerState(stage, kSamplerStateMinFilter);
    const bool minLinear = minFilter >= kTextureFilterLinear;
    desc.min_filter = minLinear ? SG_FILTER_LINEAR : SG_FILTER_NEAREST;
    const uint32_t mipFilter = states.samplerState(stage, kSamplerStateMipFilter);
    if (mipFilter == kTextureFilterNone) {
        if (desc.min_filter == SG_FILTER_LINEAR_MIPMAP_NEAREST ||
            desc.min_filter == SG_FILTER_NEAREST_MIPMAP_NEAREST ||
            desc.min_filter == SG_FILTER_LINEAR_MIPMAP_LINEAR ||
            desc.min_filter == SG_FILTER_NEAREST_MIPMAP_LINEAR) {
            desc.min_filter = minLinear ? SG_FILTER_LINEAR : SG_FILTER_NEAREST;
        }
        desc.num_mipmaps = 1;
    }
    else {
        const bool mipLinear = mipFilter >= kTextureFilterLinear;
        desc.min_filter = static_cast<sg_filter>(
            minLinear ? (mipLinear ? SG_FILTER_LINEAR_MIPMAP_LINEAR : SG_FILTER_LINEAR_MIPMAP_NEAREST)
                      : (mipLinear ? SG_FILTER_NEAREST_MIPMAP_LINEAR : SG_FILTER_NEAREST_MIPMAP_NEAREST));
        if (minFilter == kTextureFilterAnisotropic) {
            desc.max_anisotropy = 16;
        }
        desc.num_mipmaps = 0; /* actual mipmap levels decided by the image owner */
    }
    if (states.isSamplerStateExplicit(stage, kSamplerStateMipMapLODBias)) {
        note(kSamplerStateMipMapLODBias, kDispositionApproximated, "mipmap LOD bias has no sokol sink yet");
    }
    const uint32_t maxMipLevel = states.samplerState(stage, kSamplerStateMaxMipLevel);
    if (states.isSamplerStateExplicit(stage, kSamplerStateMaxMipLevel)) {
        desc.min_lod = float(maxMipLevel);
    }
    const uint32_t maxAnisotropy = states.samplerState(stage, kSamplerStateMaxAnisotropy);
    if (states.isSamplerStateExplicit(stage, kSamplerStateMaxAnisotropy)) {
        desc.max_anisotropy = maxAnisotropy < 1 ? 1 : (maxAnisotropy > 16 ? 16 : maxAnisotropy);
    }
    /* SRGBTEXTURE flips the pixel format; the caller owns formats, so surface the request
       through diagnostics rather than mutating desc.pixel_format here */
    if (states.samplerState(stage, kSamplerStateSRGBTexture) != 0) {
        note(kSamplerStateSRGBTexture, kDispositionImplemented, "sRGB sampling requires pixel format swap by owner");
    }
}

DispositionType
renderStateDisposition(uint32_t key)
{
    switch (key) {
    case kRenderStateZEnable:
    case kRenderStateZWriteEnable:
    case kRenderStateZFunc:
    case kRenderStateSrcBlend:
    case kRenderStateDestBlend:
    case kRenderStateAlphaBlendEnable:
    case kRenderStateBlendOp:
    case kRenderStateSeparateAlphaBlendEnable:
    case kRenderStateSrcBlendAlpha:
    case kRenderStateDestBlendAlpha:
    case kRenderStateBlendOpAlpha:
    case kRenderStateBlendFactor:
    case kRenderStateCullMode:
    case kRenderStateStencilEnable:
    case kRenderStateStencilFail:
    case kRenderStateStencilZFail:
    case kRenderStateStencilPass:
    case kRenderStateStencilFunc:
    case kRenderStateStencilRef:
    case kRenderStateStencilMask:
    case kRenderStateStencilWriteMask:
    case kRenderStateTwoSidedStencilMode:
    case kRenderStateCCWStencilFail:
    case kRenderStateCCWStencilZFail:
    case kRenderStateCCWStencilPass:
    case kRenderStateCCWStencilFunc:
    case kRenderStateColorWriteEnable:
    case kRenderStateColorWriteEnable1:
    case kRenderStateColorWriteEnable2:
    case kRenderStateColorWriteEnable3:
    case kRenderStateAlphaRef:
    case kRenderStateAlphaFunc:
        return kDispositionImplemented;
    case kRenderStateDepthBias:
    case kRenderStateSlopeScaleDepthBias:
    case kRenderStateFillMode:
    case kRenderStateMinTessellationLevel:
    case kRenderStateMaxTessellationLevel:
        return kDispositionApproximated;
    case kRenderStateAlphaTestEnable:
    case kRenderStateSRGBWriteEnable:
    case kRenderStateFogEnable:
    case kRenderStateLighting:
        return kDispositionShaderLevel;
    case kRenderStateScissorTestEnable:
        return kDispositionRuntimeLevel;
    case kRenderStateShadeMode:
    case kRenderStateLastPixel:
    case kRenderStateDitherEnable:
    case kRenderStateMultiSampleAntialias:
    case kRenderStateMultiSampleMask:
    case kRenderStateAntialiasedLineEnable:
    case kRenderStateDebugMonitorToken:
    case kRenderStatePatchEdgeStyle:
    case kRenderStateIndexedVertexBlendEnable:
    case kRenderStateVertexBlend:
    case kRenderStateTweenFactor:
    case kRenderStatePositionDegree:
    case kRenderStateNormalDegree:
    case kRenderStateAdaptiveTessX:
    case kRenderStateAdaptiveTessY:
    case kRenderStateAdaptiveTessZ:
    case kRenderStateAdaptiveTessW:
    case kRenderStateEnableAdaptiveTessellation:
    case kRenderStateClipping:
        return kDispositionIgnored;
    default:
        return renderStateIndexFromKey(key) >= 0 ? kDispositionShaderLevel : kDispositionUnknown;
    }
}

} /* namespace dx9rt */

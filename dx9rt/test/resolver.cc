/*
   Copyright (c) 2015-2023 hkrn All rights reserved

   This file is part of emapp component and it's licensed under Mozilla Public License. see LICENSE.md for more details.
 */

#define CATCH_CONFIG_FAST_COMPILE
#include <catch2/catch.hpp>

#include "dx9rt/Resolver.h"

using namespace dx9rt;
using namespace nanoem;

TEST_CASE("resolver-defaults")
{
    StateVector states;
    sg_pipeline_desc desc;
    ResolvedExtraStates extra;
    ResolveDiagnostics diagnostics;
    resolvePipeline(states, desc, extra, &diagnostics);
    /* D3D9 defaults: depth test+write on, LESSEQUAL */
    REQUIRE(desc.depth.write_enabled);
    REQUIRE(desc.depth.compare == SG_COMPAREFUNC_LESS_EQUAL);
    /* alpha blending off by default */
    REQUIRE_FALSE(desc.colors[0].blend.enabled);
    REQUIRE(desc.colors[0].blend.src_factor_rgb == SG_BLENDFACTOR_ONE);
    REQUIRE(desc.colors[0].blend.dst_factor_rgb == SG_BLENDFACTOR_ZERO);
    REQUIRE(desc.colors[0].blend.op_rgb == SG_BLENDOP_ADD);
    /* alpha channel mirrors rgb when separate blending is off (D3D9 default) */
    REQUIRE(desc.colors[0].blend.src_factor_alpha == desc.colors[0].blend.src_factor_rgb);
    /* stencil disabled by default, front == back */
    REQUIRE_FALSE(desc.stencil.enabled);
    REQUIRE(desc.stencil.back.compare == desc.stencil.front.compare);
    REQUIRE(desc.stencil.front.compare == SG_COMPAREFUNC_ALWAYS);
    REQUIRE(desc.stencil.front.fail_op == SG_STENCILOP_KEEP);
    /* cull CCW (D3D9 default) -> back face culling */
    REQUIRE(desc.cull_mode == SG_CULLMODE_BACK);
    /* default state produces no diagnostics: nothing silently diverges */
    REQUIRE(diagnostics.numNotes == 0);
    /* shader-level extras default off */
    REQUIRE_FALSE(extra.alphaTestEnabled);
    REQUIRE_FALSE(extra.srgbWriteEnabled);
    REQUIRE_FALSE(extra.scissorTestEnabled);
}

TEST_CASE("resolver-alpha-blend")
{
    StateVector states;
    states.setRenderState(kRenderStateAlphaBlendEnable, 1);
    states.setRenderState(kRenderStateSrcBlend, kBlendSrcAlpha);
    states.setRenderState(kRenderStateDestBlend, kBlendInvSrcAlpha);
    sg_pipeline_desc desc;
    ResolvedExtraStates extra;
    resolvePipeline(states, desc, extra, nullptr);
    REQUIRE(desc.colors[0].blend.enabled);
    REQUIRE(desc.colors[0].blend.src_factor_rgb == SG_BLENDFACTOR_SRC_ALPHA);
    REQUIRE(desc.colors[0].blend.dst_factor_rgb == SG_BLENDFACTOR_ONE_MINUS_SRC_ALPHA);
    /* separate off -> alpha mirrors rgb */
    REQUIRE(desc.colors[0].blend.src_factor_alpha == SG_BLENDFACTOR_SRC_ALPHA);

    /* BOTHSRCALPHA special pair */
    StateVector both;
    both.setRenderState(kRenderStateAlphaBlendEnable, 1);
    both.setRenderState(kRenderStateSrcBlend, kBlendBothSrcAlpha);
    resolvePipeline(both, desc, extra, nullptr);
    REQUIRE(desc.colors[0].blend.src_factor_rgb == SG_BLENDFACTOR_SRC_ALPHA);
    REQUIRE(desc.colors[0].blend.dst_factor_rgb == SG_BLENDFACTOR_ONE_MINUS_SRC_ALPHA);
}

TEST_CASE("resolver-separate-alpha-blend")
{
    StateVector states;
    states.setRenderState(kRenderStateAlphaBlendEnable, 1);
    states.setRenderState(kRenderStateSeparateAlphaBlendEnable, 1);
    states.setRenderState(kRenderStateSrcBlendAlpha, kBlendOne);
    states.setRenderState(kRenderStateDestBlendAlpha, kBlendOne);
    sg_pipeline_desc desc;
    ResolvedExtraStates extra;
    resolvePipeline(states, desc, extra, nullptr);
    REQUIRE(desc.colors[0].blend.src_factor_alpha == SG_BLENDFACTOR_ONE);
    REQUIRE(desc.colors[0].blend.dst_factor_alpha == SG_BLENDFACTOR_ONE);
}

TEST_CASE("resolver-depth-disable")
{
    StateVector states;
    states.setRenderState(kRenderStateZEnable, kZBufferFalse);
    sg_pipeline_desc desc;
    ResolvedExtraStates extra;
    resolvePipeline(states, desc, extra, nullptr);
    REQUIRE_FALSE(desc.depth.write_enabled);
    REQUIRE(desc.depth.compare == SG_COMPAREFUNC_ALWAYS);
}

TEST_CASE("resolver-two-sided-stencil")
{
    StateVector states;
    states.setRenderState(kRenderStateStencilEnable, 1);
    states.setRenderState(kRenderStateTwoSidedStencilMode, 1);
    states.setRenderState(kRenderStateStencilFunc, kCompareFuncNever);
    states.setRenderState(kRenderStateCCWStencilFunc, kCompareFuncEqual);
    states.setRenderState(kRenderStateCCWStencilFail, kStencilOperatorInvert);
    sg_pipeline_desc desc;
    ResolvedExtraStates extra;
    ResolveDiagnostics diagnostics;
    resolvePipeline(states, desc, extra, &diagnostics);
    REQUIRE(desc.stencil.enabled);
    REQUIRE(desc.stencil.front.compare == SG_COMPAREFUNC_NEVER);
    REQUIRE(desc.stencil.back.compare == SG_COMPAREFUNC_EQUAL);
    REQUIRE(desc.stencil.back.fail_op == SG_STENCILOP_INVERT);
    /* one-sided keeps back == front */
    StateVector one;
    one.setRenderState(kRenderStateStencilEnable, 1);
    one.setRenderState(kRenderStateStencilFunc, kCompareFuncNever);
    resolvePipeline(one, desc, extra, nullptr);
    REQUIRE(desc.stencil.back.compare == SG_COMPAREFUNC_NEVER);
    (void)diagnostics;
}

TEST_CASE("resolver-color-write-masks")
{
    StateVector states;
    states.setRenderState(kRenderStateColorWriteEnable, kColorWriteEnableRed | kColorWriteEnableGreen);
    states.setRenderState(kRenderStateColorWriteEnable2, 0);
    sg_pipeline_desc desc;
    ResolvedExtraStates extra;
    resolvePipeline(states, desc, extra, nullptr);
    REQUIRE((desc.colors[0].write_mask & SG_COLORMASK_R) != 0);
    REQUIRE((desc.colors[0].write_mask & SG_COLORMASK_B) == 0);
    REQUIRE(desc.colors[2].write_mask == SG_COLORMASK_NONE);
    /* attachment 1 left alone (not explicit) */
}

TEST_CASE("resolver-blend-factor-argb")
{
    StateVector states;
    states.setRenderState(kRenderStateAlphaBlendEnable, 1);
    states.setRenderState(kRenderStateSrcBlend, kBlendBlendFactor);
    states.setRenderState(kRenderStateBlendFactor, 0x00ff8040 /* A=00 R=ff G=80 B=40 */);
    sg_pipeline_desc desc;
    ResolvedExtraStates extra;
    resolvePipeline(states, desc, extra, nullptr);
    REQUIRE(desc.blend_color.r == Approx(1.0f));
    REQUIRE(desc.blend_color.g == Approx(0x80 / 255.0f));
    REQUIRE(desc.blend_color.b == Approx(0x40 / 255.0f));
    REQUIRE(desc.blend_color.a == Approx(0.0f));
}

TEST_CASE("resolver-shader-level-states")
{
    StateVector states;
    states.setRenderState(kRenderStateAlphaTestEnable, 1);
    states.setRenderState(kRenderStateAlphaRef, 128);
    states.setRenderState(kRenderStateAlphaFunc, kCompareFuncGreater);
    states.setRenderState(kRenderStateSRGBWriteEnable, 1);
    states.setRenderState(kRenderStateScissorTestEnable, 1);
    sg_pipeline_desc desc;
    ResolvedExtraStates extra;
    ResolveDiagnostics diagnostics;
    resolvePipeline(states, desc, extra, &diagnostics);
    REQUIRE(extra.alphaTestEnabled);
    REQUIRE(extra.alphaTestReference == 128);
    REQUIRE(extra.alphaTestCompareFunc == kCompareFuncGreater);
    REQUIRE(extra.srgbWriteEnabled);
    REQUIRE(extra.scissorTestEnabled);
    /* dispositions recorded, nothing silent */
    bool sawShaderLevel = false, sawRuntimeLevel = false;
    for (int i = 0; i < diagnostics.numNotes; i++) {
        sawShaderLevel |= diagnostics.notes[i].disposition == kDispositionShaderLevel;
        sawRuntimeLevel |= diagnostics.notes[i].disposition == kDispositionRuntimeLevel;
    }
    REQUIRE(sawShaderLevel);
    REQUIRE(sawRuntimeLevel);
}

TEST_CASE("resolver-sampler-image")
{
    StateVector states;
    states.setSamplerState(0, kSamplerStateAddressU, kTextureAddressClamp);
    states.setSamplerState(0, kSamplerStateAddressV, kTextureAddressMirror);
    states.setSamplerState(0, kSamplerStateMinFilter, kTextureFilterLinear);
    states.setSamplerState(0, kSamplerStateMipFilter, kTextureFilterLinear);
    states.setSamplerState(0, kSamplerStateMaxAnisotropy, 8);
    sg_image_desc desc;
    resolveSamplerImage(states, 0, desc, nullptr);
    REQUIRE(desc.wrap_u == SG_WRAP_CLAMP_TO_EDGE);
    REQUIRE(desc.wrap_v == SG_WRAP_MIRRORED_REPEAT);
    REQUIRE(desc.min_filter == SG_FILTER_LINEAR_MIPMAP_LINEAR);
    REQUIRE(desc.mag_filter == SG_FILTER_NEAREST); /* unset stays D3D default POINT */
    REQUIRE(desc.max_anisotropy == 8);
    REQUIRE(desc.num_mipmaps == 0);

    /* MIRRORONCE approximates to CLAMP_TO_EDGE with a note */
    StateVector once;
    once.setSamplerState(1, kSamplerStateAddressU, kTextureAddressMirrorOnce);
    ResolveDiagnostics diagnostics;
    resolveSamplerImage(once, 1, desc, &diagnostics);
    REQUIRE(desc.wrap_u == SG_WRAP_CLAMP_TO_EDGE);
    REQUIRE(diagnostics.numNotes == 1);
    REQUIRE(diagnostics.notes[0].disposition == kDispositionApproximated);

    /* mip off collapses min filter and pins num_mipmaps */
    StateVector noMip;
    noMip.setSamplerState(2, kSamplerStateMinFilter, kTextureFilterLinear);
    noMip.setSamplerState(2, kSamplerStateMipFilter, kTextureFilterNone);
    resolveSamplerImage(noMip, 2, desc, nullptr);
    REQUIRE(desc.min_filter == SG_FILTER_LINEAR);
    REQUIRE(desc.num_mipmaps == 1);
}

TEST_CASE("resolver-disposition-table")
{
    /* every disposition is one of the known values */
    for (int i = 0; i < kRenderStateIndexMaxEnum; i++) {
        const uint32_t key = renderStateKeyFromIndex(i);
        const DispositionType disposition = renderStateDisposition(key);
        REQUIRE(disposition != kDispositionUnknown);
    }
    REQUIRE(renderStateDisposition(0) == kDispositionUnknown);
    REQUIRE(renderStateDisposition(9999) == kDispositionUnknown);
    /* spot checks */
    REQUIRE(renderStateDisposition(kRenderStateCullMode) == kDispositionImplemented);
    REQUIRE(renderStateDisposition(kRenderStateAlphaTestEnable) == kDispositionShaderLevel);
    REQUIRE(renderStateDisposition(kRenderStateScissorTestEnable) == kDispositionRuntimeLevel);
    REQUIRE(renderStateDisposition(kRenderStateDitherEnable) == kDispositionIgnored);
    REQUIRE(renderStateDisposition(kRenderStateDepthBias) == kDispositionApproximated);
}

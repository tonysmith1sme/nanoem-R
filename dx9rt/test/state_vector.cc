/*
   Copyright (c) 2015-2023 hkrn All rights reserved

   This file is part of emapp component and it's licensed under Mozilla Public License. see LICENSE.md for more details.
 */

#define CATCH_CONFIG_FAST_COMPILE
#include <catch2/catch.hpp>

#include "dx9rt/StateVector.h"
#include "dx9rt/Types.h"

using namespace dx9rt;

TEST_CASE("state-vector-defaults")
{
    StateVector states;
    /* documented D3D9 defaults */
    REQUIRE(states.renderState(kRenderStateZEnable) == kZBufferTrue);
    REQUIRE(states.renderState(kRenderStateZWriteEnable) == 1);
    REQUIRE(states.renderState(kRenderStateCullMode) == kCullModeCCW);
    REQUIRE(states.renderState(kRenderStateZFunc) == kCompareFuncLessEqual);
    REQUIRE(states.renderState(kRenderStateAlphaBlendEnable) == 0);
    REQUIRE(states.renderState(kRenderStateSrcBlend) == kBlendOne);
    REQUIRE(states.renderState(kRenderStateDestBlend) == kBlendZero);
    REQUIRE(states.renderState(kRenderStateBlendOp) == kBlendOperatorAdd);
    REQUIRE(states.renderState(kRenderStateStencilEnable) == 0);
    REQUIRE(states.renderState(kRenderStateStencilMask) == 0xffffffffu);
    REQUIRE(states.renderState(kRenderStateColorWriteEnable) == kColorWriteEnableAll);
    REQUIRE(states.renderState(kRenderStateSRGBWriteEnable) == 0);
    REQUIRE(states.renderState(kRenderStateLighting) == 1);
    REQUIRE(decodeFloatBits(states.renderState(kRenderStateDepthBias)) == 0.0f);
    /* nothing explicit before any set call */
    REQUIRE_FALSE(states.isRenderStateExplicit(kRenderStateZEnable));
    REQUIRE_FALSE(states.isRenderStateExplicit(kRenderStateCullMode));
}

TEST_CASE("state-vector-set-and-explicit")
{
    StateVector states;
    REQUIRE(states.setRenderState(kRenderStateCullMode, kCullModeNone));
    REQUIRE(states.renderState(kRenderStateCullMode) == kCullModeNone);
    REQUIRE(states.isRenderStateExplicit(kRenderStateCullMode));
    /* explicit survives setting the same value as default */
    REQUIRE(states.setRenderState(kRenderStateZEnable, kZBufferTrue));
    REQUIRE(states.isRenderStateExplicit(kRenderStateZEnable));
    /* unknown keys are rejected, never silently swallowed */
    REQUIRE_FALSE(states.setRenderState(0, 1));
    REQUIRE_FALSE(states.setRenderState(9999, 1));
    /* wrap states are contiguous */
    REQUIRE(states.setRenderState(kRenderStateWrap0 + 7, 0x20));
    REQUIRE(states.renderState(kRenderStateWrap7) == 0x20);
    REQUIRE_FALSE(states.isRenderStateExplicit(kRenderStateWrap6));
    REQUIRE(states.isRenderStateExplicit(kRenderStateWrap7));
}

TEST_CASE("state-vector-sampler")
{
    StateVector states;
    REQUIRE(states.samplerState(0, kSamplerStateAddressU) == kTextureAddressWrap);
    REQUIRE(states.samplerState(0, kSamplerStateMagFilter) == kTextureFilterPoint);
    REQUIRE(states.samplerState(0, kSamplerStateMipFilter) == kTextureFilterNone);
    REQUIRE(states.samplerState(0, kSamplerStateMaxAnisotropy) == 1);
    REQUIRE(states.setSamplerState(3, kSamplerStateAddressV, kTextureAddressClamp));
    REQUIRE(states.samplerState(3, kSamplerStateAddressV) == kTextureAddressClamp);
    REQUIRE(states.isSamplerStateExplicit(3, kSamplerStateAddressV));
    /* other stages untouched */
    REQUIRE(states.samplerState(2, kSamplerStateAddressV) == kTextureAddressWrap);
    /* out-of-range stage and unknown key rejected */
    REQUIRE_FALSE(states.setSamplerState(kMaxSamplerCount, kSamplerStateAddressU, 1));
    REQUIRE_FALSE(states.setSamplerState(0, 0xdeadbeef, 1));
}

TEST_CASE("state-vector-hash-and-equality")
{
    StateVector a, b;
    REQUIRE(a == b);
    REQUIRE(a.hash() == b.hash());
    a.setRenderState(kRenderStateCullMode, kCullModeNone);
    REQUIRE(a != b);
    REQUIRE(a.hash() != b.hash());
    b.setRenderState(kRenderStateCullMode, kCullModeNone);
    REQUIRE(a == b);
    /* explicit flag alone differentiates (D3D treats explicit default != default) */
    b.resetToDefaults();
    b.setRenderState(kRenderStateCullMode, kCullModeCCW);
    REQUIRE(a != b);
    /* 16 stages all settable */
    StateVector c;
    for (int i = 0; i < kMaxSamplerCount; i++) {
        REQUIRE(c.setSamplerState(i, kSamplerStateMaxAnisotropy, 8));
    }
    REQUIRE(c != a);
}

TEST_CASE("state-vector-reset")
{
    StateVector states;
    states.setRenderState(kRenderStateStencilRef, 42);
    states.setSamplerState(1, kSamplerStateMagFilter, kTextureFilterLinear);
    states.resetToDefaults();
    REQUIRE(states.renderState(kRenderStateStencilRef) == 0);
    REQUIRE_FALSE(states.isRenderStateExplicit(kRenderStateStencilRef));
    REQUIRE(states.samplerState(1, kSamplerStateMagFilter) == kTextureFilterPoint);
    REQUIRE_FALSE(states.isSamplerStateExplicit(1, kSamplerStateMagFilter));
}

TEST_CASE("render-state-key-roundtrip")
{
    /* every compact index maps to a key that maps back to the same index */
    for (int i = 0; i < kRenderStateIndexMaxEnum; i++) {
        const uint32_t key = renderStateKeyFromIndex(i);
        REQUIRE(key != 0);
        REQUIRE(renderStateIndexFromKey(key) == i);
    }
    /* sampler keys 1..13 are contiguous after the address/filter table */
    uint32_t seen = 0;
    for (uint32_t key = 1; key <= 13; key++) {
        REQUIRE(samplerStateIndexFromKey(key) >= 0);
        seen |= 1u << key;
    }
    REQUIRE(seen == 0x3ffe);
    REQUIRE(samplerStateIndexFromKey(0) == -1);
    REQUIRE(samplerStateIndexFromKey(14) == -1);
}

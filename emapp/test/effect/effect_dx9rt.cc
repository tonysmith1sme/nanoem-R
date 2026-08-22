/*
   Copyright (c) 2015-2023 hkrn All rights reserved

   This file is part of emapp component and it's licensed under Mozilla Public License. see LICENSE.md for more details.
 */

#include "../common.h"

#include "emapp/effect/Common.h"

using namespace nanoem;

TEST_CASE("effect_dx9rt_apply_defaults", "[emapp][effect][dx9rt]")
{
    effect::PipelineDescriptor pd;
    /* no explicit states: the D3D9 documented defaults must land in the body */
    effect::applyDX9StateVector(pd);
    CHECK(pd.m_body.depth.write_enabled);
    CHECK(pd.m_body.depth.compare == SG_COMPAREFUNC_LESS_EQUAL);
    CHECK_FALSE(pd.m_body.colors[0].blend.enabled);
    CHECK(pd.m_body.colors[0].blend.src_factor_rgb == SG_BLENDFACTOR_ONE);
    CHECK(pd.m_body.colors[0].blend.dst_factor_rgb == SG_BLENDFACTOR_ZERO);
    CHECK_FALSE(pd.m_body.stencil.enabled);
    /* shader/runtime level flags track the vector (unset = D3D9 defaults) */
    CHECK(pd.m_hasAlphaTestEnabled);
    CHECK_FALSE(pd.m_alphaTestEnabled);
    CHECK(pd.m_hasSRGBWriteEnabled);
    CHECK_FALSE(pd.m_srgbWriteEnabled);
    CHECK(pd.m_hasScissorTestEnabled);
    CHECK_FALSE(pd.m_scissorTestEnabled);
}

TEST_CASE("effect_dx9rt_apply_explicit_states", "[emapp][effect][dx9rt]")
{
    effect::PipelineDescriptor pd;
    pd.m_stateVector.setRenderState(dx9rt::kRenderStateAlphaBlendEnable, 1);
    pd.m_stateVector.setRenderState(dx9rt::kRenderStateSrcBlend, dx9rt::kBlendSrcAlpha);
    pd.m_stateVector.setRenderState(dx9rt::kRenderStateDestBlend, dx9rt::kBlendInvSrcAlpha);
    pd.m_stateVector.setRenderState(dx9rt::kRenderStateZWriteEnable, 0);
    pd.m_stateVector.setRenderState(dx9rt::kRenderStateAlphaTestEnable, 1);
    pd.m_stateVector.setRenderState(dx9rt::kRenderStateAlphaRef, 128);
    pd.m_stateVector.setRenderState(dx9rt::kRenderStateAlphaFunc, dx9rt::kCompareFuncGreater);
    effect::applyDX9StateVector(pd);
    CHECK(pd.m_body.colors[0].blend.enabled);
    CHECK(pd.m_body.colors[0].blend.src_factor_rgb == SG_BLENDFACTOR_SRC_ALPHA);
    CHECK(pd.m_body.colors[0].blend.dst_factor_rgb == SG_BLENDFACTOR_ONE_MINUS_SRC_ALPHA);
    CHECK_FALSE(pd.m_body.depth.write_enabled);
    CHECK(pd.m_body.depth.compare == SG_COMPAREFUNC_LESS_EQUAL); /* ZEnable still on */
    CHECK(pd.m_alphaTestEnabled);
    CHECK(pd.m_alphaTestReference == 128);
    CHECK(pd.m_alphaTestCompareFunc == SG_COMPAREFUNC_GREATER);
}

TEST_CASE("effect_dx9rt_cull_mode_winding", "[emapp][effect][dx9rt]")
{
    effect::PipelineDescriptor pd;
    /* DX-convention geometry: front faces wind CW (sokol default) */
    pd.m_body.face_winding = SG_FACEWINDING_CW;
    effect::applyDX9StateVector(pd);
    CHECK(pd.m_body.cull_mode == SG_CULLMODE_BACK); /* D3D default CCW culls back faces */

    effect::PipelineDescriptor front;
    front.m_body.face_winding = SG_FACEWINDING_CCW;
    effect::applyDX9StateVector(front);
    CHECK(front.m_body.cull_mode == SG_CULLMODE_FRONT);

    effect::PipelineDescriptor none;
    none.m_stateVector.setRenderState(dx9rt::kRenderStateCullMode, dx9rt::kCullModeNone);
    none.m_body.face_winding = SG_FACEWINDING_CW;
    effect::applyDX9StateVector(none);
    CHECK(none.m_body.cull_mode == SG_CULLMODE_NONE);
}

TEST_CASE("effect_dx9rt_stencil_two_sided", "[emapp][effect][dx9rt]")
{
    effect::PipelineDescriptor pd;
    pd.m_stateVector.setRenderState(dx9rt::kRenderStateStencilEnable, 1);
    pd.m_stateVector.setRenderState(dx9rt::kRenderStateStencilFunc, dx9rt::kCompareFuncNever);
    effect::applyDX9StateVector(pd);
    CHECK(pd.m_body.stencil.enabled);
    CHECK(pd.m_body.stencil.front.compare == SG_COMPAREFUNC_NEVER);
    /* one sided mode mirrors front into back */
    CHECK(pd.m_body.stencil.back.compare == SG_COMPAREFUNC_NEVER);

    pd.m_stateVector.setRenderState(dx9rt::kRenderStateTwoSidedStencilMode, 1);
    pd.m_stateVector.setRenderState(dx9rt::kRenderStateCCWStencilFunc, dx9rt::kCompareFuncEqual);
    effect::applyDX9StateVector(pd);
    CHECK(pd.m_body.stencil.front.compare == SG_COMPAREFUNC_NEVER);
    CHECK(pd.m_body.stencil.back.compare == SG_COMPAREFUNC_EQUAL);
}

TEST_CASE("effect_dx9rt_scissor_and_srgb", "[emapp][effect][dx9rt]")
{
    effect::PipelineDescriptor pd;
    pd.m_stateVector.setRenderState(dx9rt::kRenderStateScissorTestEnable, 1);
    pd.m_stateVector.setRenderState(dx9rt::kRenderStateSRGBWriteEnable, 1);
    effect::applyDX9StateVector(pd);
    CHECK(pd.m_scissorTestEnabled);
    CHECK(pd.m_srgbWriteEnabled);
    /* explicit D3D9 color write default keeps all channels on */
    pd.m_stateVector.setRenderState(dx9rt::kRenderStateColorWriteEnable, dx9rt::kColorWriteEnableRed);
    effect::applyDX9StateVector(pd);
    CHECK((pd.m_body.colors[0].write_mask & SG_COLORMASK_R) != 0);
    CHECK((pd.m_body.colors[0].write_mask & SG_COLORMASK_G) == 0);
}

TEST_CASE("effect_dx9rt_sampler_defaults", "[emapp][effect][dx9rt]")
{
    dx9rt::StateVector states;
    sg_image_desc desc;
    effect::applyDX9SamplerStateVector(states, 0, desc);
    /* documented D3D9 sampler defaults */
    CHECK(desc.mag_filter == SG_FILTER_NEAREST);
    CHECK(desc.min_filter == SG_FILTER_NEAREST);
    CHECK(desc.wrap_u == SG_WRAP_REPEAT);
    CHECK(desc.wrap_v == SG_WRAP_REPEAT);
    CHECK(desc.border_color == SG_BORDERCOLOR_TRANSPARENT_BLACK);
    CHECK(desc.num_mipmaps == 1); /* MIPFILTER NONE */

    dx9rt::StateVector linear;
    linear.setSamplerState(0, dx9rt::kSamplerStateMagFilter, dx9rt::kTextureFilterLinear);
    linear.setSamplerState(0, dx9rt::kSamplerStateMinFilter, dx9rt::kTextureFilterLinear);
    linear.setSamplerState(0, dx9rt::kSamplerStateMipFilter, dx9rt::kTextureFilterLinear);
    linear.setSamplerState(0, dx9rt::kSamplerStateAddressU, dx9rt::kTextureAddressClamp);
    effect::applyDX9SamplerStateVector(linear, 0, desc);
    CHECK(desc.mag_filter == SG_FILTER_LINEAR);
    CHECK(desc.min_filter == SG_FILTER_LINEAR_MIPMAP_LINEAR);
    CHECK(desc.wrap_u == SG_WRAP_CLAMP_TO_EDGE);
    CHECK(desc.num_mipmaps == 0);
}

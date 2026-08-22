/*
   Copyright (c) 2015-2023 hkrn All rights reserved

   This file is part of emapp component and it's licensed under Mozilla Public License. see LICENSE.md for more details.
 */

/* FXAA 3.11 derived implementation.
   Copyright (c) 2010-2011 Timothy Lottes, NVIDIA Corporation.
   Permission is hereby granted, free of charge, to any person obtaining a copy of this
   software and associated documentation files (the "Software"), to deal in the Software
   without restriction, including without limitation the rights to use, copy, modify,
   merge, publish, distribute, sublicense, and/or sell copies of the Software, and to
   permit persons to whom the Software is furnished to do so, subject to the following
   conditions: The above copyright notice and this permission notice shall be included
   in all copies or substantial portions of the Software. THE SOFTWARE IS PROVIDED "AS
   IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, IN NO EVENT SHALL THE AUTHORS
   OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
   AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH
   THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE. */

#include "nanoem/io.hlsl"

cbuffer fxaa_parameters_t : register(b0) {
    /* xy = 1.0 / size of the source image, zw = size of the source image */
    float4 u_texelSize;
};

GLSLANG_ANNOTATION([[vk::binding(0, VK_DESCRIPTOR_SET_TEXTURE)]])
Texture2D u_texture : register(t0);
GLSLANG_ANNOTATION([[vk::binding(1, VK_DESCRIPTOR_SET_SAMPLER)]])
SamplerState u_textureSampler : register(s0);

float
FxaaLuma(float3 rgb)
{
    return dot(rgb, float3(0.299, 0.587, 0.114));
}

float3
FxaaSample(float2 uv)
{
    return u_texture.Sample(u_textureSampler, uv).rgb;
}

float
FxaaSampleLuma(float2 uv)
{
    return FxaaLuma(FxaaSample(uv));
}

vs_output_t
nanoemVSMain(vs_input_t input)
{
    vs_output_t output;
    output.position = float4(input.position, 1);
    output.texcoord0 = input.texcoord0;
    return output;
}

float4
nanoemPSMain(ps_input_t input) : SV_TARGET0
{
    const float2 uv = input.texcoord0;
    const float2 texel = u_texelSize.xy;
    const float lumaM = FxaaSampleLuma(uv);
    const float lumaN = FxaaSampleLuma(uv + float2(0, -1) * texel);
    const float lumaS = FxaaSampleLuma(uv + float2(0, 1) * texel);
    const float lumaE = FxaaSampleLuma(uv + float2(1, 0) * texel);
    const float lumaW = FxaaSampleLuma(uv + float2(-1, 0) * texel);
    const float lumaMin = min(lumaM, min(min(lumaN, lumaS), min(lumaE, lumaW)));
    const float lumaMax = max(lumaM, max(max(lumaN, lumaS), max(lumaE, lumaW)));
    /* early out on flat areas to keep the pass cheap */
    if (lumaMax - lumaMin < max(1.0 / 128.0, lumaMax * (1.0 / 16.0))) {
        return float4(FxaaSample(uv), 1);
    }
    /* estimate the edge direction (horizontal when the vertical gradient dominates) */
    const float range = lumaMax - lumaMin;
    const float lumaNE = FxaaSampleLuma(uv + float2(1, -1) * texel);
    const float lumaSW = FxaaSampleLuma(uv + float2(-1, 1) * texel);
    const float lumaNS = lumaN + lumaS;
    const float lumaEW = lumaE + lumaW;
    const float lumaNE_SW = lumaNE + lumaSW;
    const float lumaNW = lumaN + lumaW + lumaM + lumaNE;
    const float lumaSE = lumaS + lumaE + lumaM + lumaSW;
    const bool isHorizontal = abs(lumaNW - lumaSE) * 2.0 > lumaNS + lumaEW - lumaNE_SW;
    const float lumaDirA = isHorizontal ? lumaN : lumaE;
    const float dir = lumaDirA - lumaM >= 0 ? 1.0 : -1.0;
    /* walk along the edge to both ends and locate the local contrast span */
    const float2 dirStep = (isHorizontal ? float2(1, 0) : float2(0, 1)) * dir * texel;
    float2 uv1 = uv + dirStep * (3.0 / 4.0);
    float luma1 = FxaaSampleLuma(uv1);
    float2 uv2 = uv - dirStep * (3.0 / 4.0);
    float luma2 = FxaaSampleLuma(uv2);
    const float lumaEnd1 = luma1;
    const float lumaEnd2 = luma2;
    float lumaEndMin = min(lumaEnd1, lumaEnd2);
    float lumaEndMax = max(lumaEnd1, lumaEnd2);
    [unroll]
    for (int i = 0; i < 4; i++) {
        if (abs(lumaEndMax - lumaEndMin) > range * 0.25) {
            break;
        }
        luma1 = FxaaSampleLuma(uv1 + dirStep);
        luma2 = FxaaSampleLuma(uv2 - dirStep);
        uv1 += dirStep;
        uv2 -= dirStep;
        lumaEndMin = min(min(luma1, luma2), lumaEndMin);
        lumaEndMax = max(max(luma1, luma2), lumaEndMax);
    }
    /* pick the closer end and the subpixel blend weight */
    const float dist1 = abs(uv1.x - uv.x) + abs(uv1.y - uv.y);
    const float dist2 = abs(uv2.x - uv.x) + abs(uv2.y - uv.y);
    const bool is1Closer = dist1 < dist2;
    const float dist = min(dist1, dist2);
    const float pixelDist = isHorizontal ? texel.x : texel.y;
    const float subpixelOffset = clamp(dist / pixelDist - 0.5, 0, 1.5);
    const float lumaAvg = (lumaNS + lumaEW + lumaNE_SW * 2.0) * (1.0 / 12.0);
    const float subpixelWeight = clamp(abs(lumaAvg - lumaM) / range, 0, 1);
    const float blend = max(subpixelOffset * subpixelWeight, 0) * (is1Closer ? -1.0 : 1.0);
    return float4(FxaaSample(uv + dirStep * blend), 1);
}

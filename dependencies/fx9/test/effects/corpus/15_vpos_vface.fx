// VPOS and VFACE pixel input semantics
float4 ps_main(float4 color : COLOR0, float2 vpos : VPOS, float vface : VFACE) : COLOR0
{
    float checker = fmod(floor(vpos.x / 8) + floor(vpos.y / 8), 2);
    return color * lerp(0.9, 1.1, checker) * (vface > 0 ? 1 : 0.5);
}
float4 vs_main(float4 position : POSITION) : POSITION { return position; }
technique vpos_vface {
  pass main {
    VertexShader = compile vs_3_0 vs_main();
    PixelShader = compile ps_3_0 ps_main();
  }
}

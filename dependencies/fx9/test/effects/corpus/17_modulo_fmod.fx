// % on floats (rewritten to trunc(fmod)) and fmod builtin
float4 ps_main(float4 color : COLOR0) : COLOR0
{
    float a = fmod(color.r * 100, 7);
    float b = (color.g * 100) % 7;
    float2 c = float2(a, b);
    return float4(c * (1.0 / 7.0), 0, 1);
}
float4 vs_main(float4 position : POSITION) : POSITION { return position; }
technique modulo_fmod {
  pass main {
    VertexShader = compile vs_3_0 vs_main();
    PixelShader = compile ps_3_0 ps_main();
  }
}

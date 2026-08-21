// float4x4 to float3x3 cast truncation (used by ray-mmd, known unsupported, baseline fail)
float4x4 g_world : WORLD;
float4 vs_main(float4 position : POSITION, out float3 onormal : TEXCOORD0) : POSITION
{
    onormal = mul(float3(0, 1, 0), (float3x3) g_world);
    return position;
}
float4 ps_main(float3 normal : TEXCOORD0) : COLOR0 { return float4(normal, 1); }
technique matrix_cast {
  pass main {
    VertexShader = compile vs_3_0 vs_main();
    PixelShader = compile ps_3_0 ps_main();
  }
}

// matrix and light semantics with multiple samplers
float4x4 g_world : WORLD;
float4x4 g_viewproj : VIEWPROJECTION;
float3x3 g_world3x3 : WORLD;
float4 g_lightdir : LIGHTDIRECTION;
sampler2D g_samp0 : MATERIALTEXTURE;
sampler2D g_samp1 : SPHEREMAP;
float4 vs_main(float4 position : POSITION, float3 normal : NORMAL, out float3 onormal : TEXCOORD0) : POSITION
{
    onormal = normalize(mul(normal, g_world3x3));
    return mul(position, mul(g_world, g_viewproj));
}
float4 ps_main(float3 normal : TEXCOORD0) : COLOR0
{
    float l = saturate(dot(normalize(g_lightdir.xyz), normal));
    return lerp(float4(0.2, 0.2, 0.2, 1), tex2D(g_samp0, normal.xy * 0.5 + 0.5), l) + tex2D(g_samp1, normal.zx * 0.5 + 0.5) * 0.001;
}
technique matrix_semantics {
  pass main {
    VertexShader = compile vs_3_0 vs_main();
    PixelShader = compile ps_3_0 ps_main();
  }
}

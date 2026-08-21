// matrix element swizzle (_m00 style) and scalar matrix init
float4x4 g_matrix = 0.0;
float3x3 makeRotation(float angle)
{
    float s = sin(angle), c = cos(angle);
    float3x3 m = 0.0;
    m._m00 = c; m._m02 = s;
    m._m11 = 1;
    m._m20 = -s; m._m22 = c;
    return m;
}
float4 vs_main(float4 position : POSITION) : POSITION
{
    float3x3 r = makeRotation(0.5);
    float3 p = mul(position.xyz, r);
    float2 e = g_matrix._m00_m11;
    return float4(p + float3(e, 0), 1);
}
float4 ps_main() : COLOR0 { return float4(1, 1, 1, 1); }
technique matrix_swizzle {
  pass main {
    VertexShader = compile vs_3_0 vs_main();
    PixelShader = compile ps_3_0 ps_main();
  }
}

// tex2Dlod / tex2Dbias / tex2Dproj variants
sampler2D samp < string ResourcePath = "*"; >;
float4 ps_variants(float4 texcoord : TEXCOORD0) : COLOR0
{
    float4 a = tex2Dlod(samp, float4(texcoord.xy, 0, 0));
    float4 b = tex2Dbias(samp, float4(texcoord.xy, 0, -0.5));
    float4 c = tex2Dproj(samp, texcoord);
    return a + b + c;
}
float4 vs_main(float4 position : POSITION) : POSITION { return mul(position, float4(1, 1, 1, 1)); }
technique variants {
  pass main {
    VertexShader = compile vs_3_0 vs_main();
    PixelShader = compile ps_3_0 ps_variants();
  }
}

// 17 samplers exceed the 16 register limit (s0..s15) and must fail loudly
sampler2D samp0 : register(s0);
sampler2D samp1 : register(s1);
sampler2D samp2 : register(s2);
sampler2D samp3 : register(s3);
sampler2D samp4 : register(s4);
sampler2D samp5 : register(s5);
sampler2D samp6 : register(s6);
sampler2D samp7 : register(s7);
sampler2D samp8 : register(s8);
sampler2D samp9 : register(s9);
sampler2D samp10 : register(s10);
sampler2D samp11 : register(s11);
sampler2D samp12 : register(s12);
sampler2D samp13 : register(s13);
sampler2D samp14 : register(s14);
sampler2D samp15 : register(s15);
sampler2D samp16 : register(s16);
float4 ps_many(float4 t : TEXCOORD0) : COLOR0
{
    float4 c = 0;
    c += tex2D(samp0, t.xy) * 0.058824;
    c += tex2D(samp1, t.xy) * 0.058824;
    c += tex2D(samp2, t.xy) * 0.058824;
    c += tex2D(samp3, t.xy) * 0.058824;
    c += tex2D(samp4, t.xy) * 0.058824;
    c += tex2D(samp5, t.xy) * 0.058824;
    c += tex2D(samp6, t.xy) * 0.058824;
    c += tex2D(samp7, t.xy) * 0.058824;
    c += tex2D(samp8, t.xy) * 0.058824;
    c += tex2D(samp9, t.xy) * 0.058824;
    c += tex2D(samp10, t.xy) * 0.058824;
    c += tex2D(samp11, t.xy) * 0.058824;
    c += tex2D(samp12, t.xy) * 0.058824;
    c += tex2D(samp13, t.xy) * 0.058824;
    c += tex2D(samp14, t.xy) * 0.058824;
    c += tex2D(samp15, t.xy) * 0.058824;
    c += tex2D(samp16, t.xy) * 0.058824;
    return c;
}
float4 vs_main(float4 position : POSITION) : POSITION { return position; }
technique many_samplers {
  pass main {
    VertexShader = compile vs_3_0 vs_main();
    PixelShader = compile ps_3_0 ps_many();
  }
}

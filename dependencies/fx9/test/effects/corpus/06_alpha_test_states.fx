// alpha test states drive runtime discard emulation
float4 vs_main(float4 position : POSITION, float2 texcoord : TEXCOORD0, out float2 otexcoord : TEXCOORD0) : POSITION
{
    otexcoord = texcoord;
    return position;
}
float4 ps_main(float4 color : COLOR0) : COLOR0 { return color; }
technique alpha_test < string ScriptClass = "object"; > {
  pass main {
    AlphaTestEnable = true;
    AlphaRef = 128;
    AlphaFunc = Greater;
    SrcBlend = SrcAlpha;
    DestBlend = InvSrcAlpha;
    AlphaBlendEnable = true;
    VertexShader = compile vs_3_0 vs_main();
    PixelShader = compile ps_3_0 ps_main();
  }
}

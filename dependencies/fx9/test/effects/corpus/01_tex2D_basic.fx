// basic sampler and tex2D usage
texture2D diffusetex : DIFFUSETURE < string ResourcePath = "*"; >;
sampler2D diffsamp = sampler_state {
    Texture = <diffusetex>;
    MinFilter = Linear; MagFilter = Linear; MipFilter = Linear;
    AddressU = Clamp; AddressV = Clamp;
};
float4 ps_tex2d(float4 texcoord : TEXCOORD0) : COLOR0
{
    return tex2D(diffsamp, texcoord.xy);
}
float4 vs_main(float4 position : POSITION) : POSITION { return position; }
technique tex2d_basic < string ScriptClass = "scene"; > {
  pass main {
    VertexShader = compile vs_3_0 vs_main();
    PixelShader = compile ps_3_0 ps_tex2d();
  }
}

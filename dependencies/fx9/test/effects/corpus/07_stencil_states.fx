// stencil state coverage
float4 vs_main(float4 position : POSITION) : POSITION { return position; }
float4 ps_main() : COLOR0 { return float4(1, 1, 1, 1); }
technique stencil {
  pass main {
    StencilEnable = true;
    StencilFunc = LessEqual;
    StencilRef = 3;
    StencilMask = 0xFF;
    StencilWriteMask = 0xFF;
    StencilFail = Keep;
    StencilZFail = Incr;
    StencilPass = Replace;
    ZEnable = true;
    ZWriteEnable = true;
    ZFunc = LessEqual;
    CullMode = CCW;
    VertexShader = compile vs_3_0 vs_main();
    PixelShader = compile ps_3_0 ps_main();
  }
}

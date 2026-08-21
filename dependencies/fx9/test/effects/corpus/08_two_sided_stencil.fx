// two sided stencil with CCW variants
float4 vs_main(float4 position : POSITION) : POSITION { return position; }
float4 ps_main() : COLOR0 { return float4(1, 1, 1, 1); }
technique two_sided {
  pass main {
    TwoSidedStencilMode = true;
    StencilEnable = true;
    StencilFunc = Always;
    CCW_StencilFunc = Never;
    CCW_StencilFail = Decr;
    CCW_StencilZFail = Keep;
    CCW_StencilPass = Invert;
    VertexShader = compile vs_3_0 vs_main();
    PixelShader = compile ps_3_0 ps_main();
  }
}

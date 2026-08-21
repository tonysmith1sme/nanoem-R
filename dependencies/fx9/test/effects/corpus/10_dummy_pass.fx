// script-only pass without shaders (regression seed: must count as compiled)
float4 vs_main(float4 position : POSITION) : POSITION { return position; }
float4 ps_main() : COLOR0 { return float4(1, 1, 1, 1); }
technique dummy_pass {
  pass RealDraw {
    VertexShader = compile vs_3_0 vs_main();
    PixelShader = compile ps_3_0 ps_main();
  }
  pass StateOnly < string Script = "Draw=Geometry;"; > {
    ZWriteEnable = false;
    AlphaBlendEnable = true;
  }
}

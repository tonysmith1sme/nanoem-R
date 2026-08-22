// ## token pasting in identifiers and semantics (supported; number pasting is not)
#define CONCAT(a, b) a##b
float4 CONCAT(my, var)() : CONCAT(POS, ITION) { return float4(1, 1, 1, 1); }
float4 vs_main(float4 position : POSITION) : POSITION { return myvar() + position; }
float4 ps_main() : COLOR0 { return myvar(); }
technique token_pasting {
  pass main {
    VertexShader = compile vs_3_0 vs_main();
    PixelShader = compile ps_3_0 ps_main();
  }
}

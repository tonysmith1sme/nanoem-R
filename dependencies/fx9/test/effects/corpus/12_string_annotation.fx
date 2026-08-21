// string list annotations and UI metadata
float4 Param
<
  string UIName = "Parameter Name";
  string UIWidget = "Slider";
  string UIHelp = "help line 1" "help line 2";
  bool UIVisible = true;
  float UIMin = 0.0;
  float UIMax = 1.0;
> = float4(0.5, 0.5, 0.5, 1.0);
float4 vs_main(float4 position : POSITION) : POSITION { return position; }
float4 ps_main() : COLOR0 { return Param; }
technique string_annotation {
  pass main {
    VertexShader = compile vs_3_0 vs_main();
    PixelShader = compile ps_3_0 ps_main();
  }
}

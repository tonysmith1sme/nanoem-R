// ternary, bool arithmetic and vector compares
float4 g_flag < bool UIVisible = false; > = false;
float4 ps_main(float4 color : COLOR0) : COLOR0
{
    bool active = g_flag.r > 0.5 ? true : false;
    float weight = active ? 1 : 0.25;
    bool2 mask = color.rg > 0.5;
    float2 m = mask ? 1 : 0;
    return color * weight * float4(m, 1, 1);
}
float4 vs_main(float4 position : POSITION) : POSITION { return position; }
technique ternary_bool {
  pass main {
    VertexShader = compile vs_3_0 vs_main();
    PixelShader = compile ps_3_0 ps_main();
  }
}

// struct IO plumbing in entry points (regression seed for vs_output_t collision)
struct vs_output_t {
    float4 position : POSITION;
    float4 color0 : COLOR0;
    float2 texcoord0 : TEXCOORD0;
};
struct vs_input_t {
    float4 position : POSITION;
    float4 color0 : COLOR0;
    float2 texcoord0 : TEXCOORD0;
};
vs_output_t vs_main(vs_input_t input)
{
    vs_output_t output = (vs_output_t) 0;
    output.position = input.position;
    output.color0 = input.color0;
    output.texcoord0 = input.texcoord0;
    return output;
}
float4 ps_main(float4 color0 : COLOR0, float2 texcoord0 : TEXCOORD0) : COLOR0
{
    return color0 * float4(texcoord0, 0, 1);
}
technique struct_io {
  pass main {
    VertexShader = compile vs_3_0 vs_main();
    PixelShader = compile ps_3_0 ps_main();
  }
}

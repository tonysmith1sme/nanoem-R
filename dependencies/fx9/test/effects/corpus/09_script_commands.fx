// MME script command annotations (LoopByCount / Clear / Draw=Buffer)
sampler2D scenetex : MATERIALTEXTURE;
float4 vs_buffer(float4 position : POSITION, out float2 texcoord : TEXCOORD0) : POSITION
{
    texcoord = position.xy * 0.5 + 0.5;
    return float4(position.x, -position.y, 0, 1);
}
float4 ps_buffer(float2 texcoord : TEXCOORD0) : COLOR0 { return tex2D(scenetex, texcoord); }
float4 ps_clear() : COLOR0 { return 0; }
technique script_commands
<
  string ScriptClass = "scene";
  string Script =
    "Pass=Setup;"
    "LoopByCount=2;"
    "Pass=DrawBuffer;"
    "LoopEnd;"
    "Pass=Finish;";
> {
  pass Setup < string Script = "Clear=Color; ClearSetColor=1.0,1.0,1.0,1.0; Clear=Depth; ClearSetDepth=1.0;"; > {
    ZEnable = false;
    VertexShader = compile vs_3_0 vs_buffer();
    PixelShader = compile ps_3_0 ps_clear();
  }
  pass DrawBuffer < string Script = "Draw=Buffer;"; > {
    VertexShader = compile vs_3_0 vs_buffer();
    PixelShader = compile ps_3_0 ps_buffer();
  }
  pass Finish < string Script = "Draw=Buffer;"; > {
    SRGBWriteEnable = true;
    VertexShader = compile vs_3_0 vs_buffer();
    PixelShader = compile ps_3_0 ps_buffer();
  }
}

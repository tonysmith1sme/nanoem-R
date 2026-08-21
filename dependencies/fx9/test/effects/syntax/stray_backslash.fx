bool Dummy <
   string UIName = "AO\표시";
   bool UIVisible = true;
> = false;

float4 vs_stray_backslash(float4 position : POSITION) : POSITION
{
	return position;
}

float4 ps_stray_backslash(float4 color : COLOR0) : COLOR0
{
	return color * (Dummy ? 1 : 0);
}

technique stray_backslash_technique {
  pass stray_backslash {
  	VertexShader = compile vs_3_0 vs_stray_backslash();
  	PixelShader = compile vs_3_0 ps_stray_backslash();
  }
}

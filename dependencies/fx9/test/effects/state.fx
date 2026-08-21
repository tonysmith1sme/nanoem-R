float4 stub_vs(float4 position : POSITION) : POSITION
{
	return float4(0, 0, 0, 1);
}

float4 stub_ps() : COLOR0
{
	return float4(1, 1, 1, 1);
}

technique state_test_technique {
  pass all_states {
  	ZEnable = false;
  	FillMode = Solid;
  	ShadeMode = Gouraud;
  	ZWriteEnable = true;
  	AlphaTestEnable = true;
  	LastPixel = true;
  	SrcBlend = SrcAlpha;
  	DestBlend = InvSrcAlpha;
  	CullMode = CCW;
  	ZFunc = LessEqual;
  	AlphaRef = 0.5;
  	AlphaFunc = Greater;
  	DitherEnable = false;
  	AlphaBlendEnable = false;
  	FogEnable = false;
  	SpecularEnable = false;
  	FogColor = 0xFF808080;
  	FogTableMode = Linear;
  	FogStart = 0.0;
  	FogEnd = 1.0;
  	FogDensity = 0.5;
  	RangeFogEnable = false;
  	StencilEnable = false;
  	StencilFail = Keep;
  	StencilZFail = Incr;
  	StencilPass = Replace;
  	StencilFunc = Always;
  	StencilRef = 1;
  	StencilMask = 0xFFFFFFFF;
  	StencilWriteMask = 0xFFFFFFFF;
  	TextureFactor = 0xFFFFFFFF;
  	Wrap0 = U | V;
  	Wrap1 = U;
  	Wrap2 = V;
  	Wrap3 = 0;
  	Wrap4 = 0;
  	Wrap5 = 0;
  	Wrap6 = 0;
  	Wrap7 = 0;
  	Clipping = true;
  	Lighting = false;
  	Ambient = 0x00202020;
  	FogVertexMode = Linear;
  	ColorVertex = true;
  	LocalViewer = true;
  	NormalizeNormals = true;
  	DiffuseMaterialSource = Material;
  	SpecularMaterialSource = Material;
  	AmbientMaterialSource = Material;
  	EmissiveMaterialSource = Material;
  	ClipPlaneEnable = 0;
  	PointSize = 1.0;
  	PointSize_Min = 0.0;
  	PointSize_Max = 1.0;
  	PointSpriteEnable = false;
  	PointScaleEnable = false;
  	PointScale_A = 1.0;
  	PointScale_B = 0.0;
  	PointScale_C = 0.0;
  	MultiSampleAliases = false;
  	MultiSampleMask = 0xFFFFFFFF;
  	PatchEdgeStyle = Discrete;
  	DebugMonitorToken = 0;
  	IndexedVertexBlendEnable = false;
  	TweenFactor = 0.5;
  	BlendOp = Add;
  	PositionDegree = 1.0;
  	NormalDegree = 1.0;
  	ScissorTestEnable = false;
  	SlopeScaleDepthBias = 0.0;
  	AntialiasedLineEnable = false;
  	MinTessellationLevel = 0;
  	MaxTessellationLevel = 0;
  	Adaptiveness_X = 0;
  	Adaptiveness_Y = 0;
  	Adaptiveness_Z = 0;
  	Adaptiveness_W = 0;
  	EnableAdaptiveTessellation = false;
  	TwoSidedStencilMode = false;
  	CCW_StencilFail = Keep;
  	CCW_StencilZFail = Incr;
  	CCW_StencilPass = Replace;
  	ColorWhiteEnable1 = false;
  	ColorWhiteEnable2 = false;
  	ColorWhiteEnable3 = false;
  	BlendFactor = 0xFFFFFFFF;
  	SRGBWriteEnable = false;
  	DepthBias = 0.0;
  	Wrap8 = 0;
  	Wrap9 = 0;
  	Wrap10 = 0;
  	Wrap11 = 0;
  	Wrap12 = 0;
  	Wrap13 = 0;
  	Wrap14 = 0;
  	Wrap15 = 0;
  	SeparateAlphaBlendEnable = false;
  	SrcBlendAlpha = SrcAlpha;
  	DestBlendAlpha = InvSrcAlpha;
  	BlendOpAlpha = Add;
  	VertexShader = compile vs_3_0 stub_vs();
  	PixelShader = compile vs_3_0 stub_ps();
  }
}

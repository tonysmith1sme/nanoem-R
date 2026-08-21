// texM3x3* family (known unsupported, baseline fail)
sampler2D samp;
samplerCUBE csamp;
struct vsio {
    float4 position : POSITION;
    float3 tangent : TEXCOORD0;
    float3 binormal : TEXCOORD1;
    float3 normal : TEXCOORD2;
};
float4 mainvs(float4 position : POSITION, float3 tangent : TEXCOORD0, float3 binormal : TEXCOORD1,
    float3 normal : TEXCOORD2, out float3 oTangent : TEXCOORD0, out float3 oBinormal : TEXCOORD1,
    out float3 oNormal : TEXCOORD2) : POSITION
{
    oTangent = tangent; oBinormal = binormal; oNormal = normal;
    return position;
}
float4 mainps(float4 texcoord0 : TEXCOORD0, float4 texcoord1 : TEXCOORD1, float4 texcoord2 : TEXCOORD2) : COLOR0
{
    float3x3 m;
    m._m00_m10_m20 = texcoord0.xyz;
    m._m01_m11_m21 = texcoord1.xyz;
    m._m02_m12_m22 = texcoord2.xyz;
    return texM3x3(samp, texcoord0.xyz, texcoord1.xyz, texcoord2.xyz) + texM3x3vspec(samp, texcoord0.xyz, texcoord1.xyz, texcoord2.xyz, float3(0, 0, 1));
}
technique texm3x3 {
  pass main {
    VertexShader = compile vs_3_0 mainvs();
    PixelShader = compile ps_3_0 mainps();
  }
}

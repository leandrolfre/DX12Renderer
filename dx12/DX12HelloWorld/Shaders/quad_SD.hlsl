

struct VertexIn
{
	float3 PosL   	: POSITION;
	float3 Tangent  : TANGENT;
	float3 Normal 	: NORMAL;
	float2 Tex0   	: TEX0;
	float2 Tex1   	: TEX1;
};

struct VertexOut
{
	float4 PosH  : SV_POSITION;
	float2 TexCoord : TEXCOORD;
};

cbuffer cbSettings : register(b0)
{
	float test;
}

Texture2D gScreenMap : register(t0);
SamplerState gLinearSample : register(s0);

VertexOut VS(VertexIn vin, float4 Color : COLOR)
{
	VertexOut vout;
	
	// Transform to homogeneous clip space.
	vout.PosH = float4(vin.PosL, 1.0f);
	vout.TexCoord = vin.Tex0;
    return vout;
}

float4 PS(VertexOut pin) : SV_Target
{
	float4 diffuse = gScreenMap.Sample(gLinearSample, pin.TexCoord);
	return diffuse;
	//return float4(test, 0.0f, 0.0f, 1.0f);
}
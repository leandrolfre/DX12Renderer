#include "inc/Common.hlsli"

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
};

VertexOut VS(VertexIn vin, float4 Color : COLOR)
{
	VertexOut vout;
	
	float4 posW = mul(gWorld, float4(vin.PosL, 1.0f));
	vout.PosH = mul(gLights[0].ShadowViewProj, posW);
	
    return vout;
}

void PS(VertexOut pin)
{
}
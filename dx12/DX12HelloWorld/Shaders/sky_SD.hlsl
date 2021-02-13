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
	float4 PosH : SV_POSITION;
	float3 PosL : POSITION;
};

VertexOut VS(VertexIn vin)
{
	VertexOut vout;
	vout.PosL = vin.PosL;
	
	float4 PosW = mul(gWorld, float4(vin.PosL, 1.0f));
	PosW.xyz += gEyePosW;
	
	vout.PosH = mul(gViewProj, PosW).xyww;
	return vout;
}

float4 PS(VertexOut pin) : SV_TARGET
{
	return pow(gCubeMap.Sample(gLinearSample, pin.PosL), 2.2);
}
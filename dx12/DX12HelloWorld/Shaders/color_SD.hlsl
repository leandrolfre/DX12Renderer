
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
	float4 PosH			: SV_POSITION;
	float3 PosV			: POSITION;
	float4 ShadowPosH	: POSITION1;
    float3 NormalV		: NORMAL;
	float2 TexCoord		: TEXCOORD;
	float3 TangentV		: TANGENT;
	float3 BitangentV	: Binormal;
};

struct GBuffer
{
	float4 Position : SV_TARGET0;
	float4 Normal : SV_TARGET1;
	float4 Albedo : SV_TARGET2;
	float4 FresnelShininess : SV_TARGET3;
};

VertexOut VS(VertexIn vin, float4 Color : COLOR)
{
	VertexOut vout;
	
	float4 posW = mul(gWorld, float4(vin.PosL, 1.0f));
	vout.PosV = mul(gView, posW).xyz;
	
	// Transform to homogeneous clip space.
	vout.PosH = mul(gViewProj, posW);
	vout.ShadowPosH = mul(gLights[0].ShadowTransform, posW);
	vout.NormalV = mul((float3x3)gView, mul((float3x3)gWorld, vin.Normal));
	vout.TangentV = mul((float3x3)gView, mul((float3x3)gWorld, vin.Tangent));
	vout.BitangentV = cross(vout.NormalV, vout.TangentV);
	vout.TexCoord = vin.Tex0;

    return vout;
}

GBuffer PS(VertexOut pin)
{
	MaterialData matData = gMaterialData[gMaterialIndex];

    pin.NormalV = normalize(pin.NormalV);

	if (matData.hasNormalMap)
	{
		float3 N = pin.NormalV;
		float3 T = normalize(pin.TangentV - dot(pin.TangentV, N) * N);
		float3 B = normalize(pin.BitangentV - dot(pin.BitangentV, N) * N);
		
		float3x3 TBN = { T, B, N };
		//float4 normalMap = gMaterialMap[matData.DiffuseMapIndex + 1].Sample(gLinearSample, pin.TexCoord);
		float4 normalMap = gNormalMap.Sample(gLinearSample, pin.TexCoord);
		normalMap = normalMap * 2.0f - 1.0f;
		pin.NormalV = mul(normalMap.xyz, TBN);
	}

	float4 diffuse = pow(gDiffuseMap.Sample(gLinearSample, pin.TexCoord), 2.2f);
	const float shininess = 1.0f - matData.Roughness;
	
	GBuffer gbuffer;
	gbuffer.Position = float4(pin.PosV, 1.0f);
	gbuffer.Normal = float4(pin.NormalV, 1.0f);
	gbuffer.Albedo = diffuse;
	gbuffer.FresnelShininess = float4(matData.FresnelR0.xyz, shininess);
	
	return gbuffer;
}
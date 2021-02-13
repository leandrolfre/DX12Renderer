
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
	float3 PosW			: POSITION;
	float4 ShadowPosH	: POSITION1;
    float3 NormalW		: NORMAL;
	float2 TexCoord		: TEXCOORD;
	float3 TangentW		: TANGENT;
	float3 BitangentW	: Binormal;
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
	vout.PosW = posW.xyz;
	
	// Transform to homogeneous clip space.
	vout.PosH = mul(gViewProj, posW);
	vout.ShadowPosH = mul(gLights[0].ShadowTransform, posW);
	vout.NormalW = mul((float3x3)gWorld, vin.Normal);
	vout.TangentW = mul((float3x3)gWorld, vin.Tangent);
	vout.BitangentW = cross(vout.NormalW, vout.TangentW);
	vout.TexCoord = vin.Tex0;
    return vout;
}

GBuffer PS(VertexOut pin)
{
	MaterialData matData = gMaterialData[gMaterialIndex];

    pin.NormalW = normalize(pin.NormalW);

	if (matData.hasNormalMap)
	{
		float3 N = pin.NormalW;
		float3 T = normalize(pin.TangentW - dot(pin.TangentW, N) * N);
		float3 B = normalize(pin.BitangentW - dot(pin.BitangentW, N) * N);
		
		float3x3 TBN = { T, B, N };
		//float4 normalMap = gMaterialMap[matData.DiffuseMapIndex + 1].Sample(gLinearSample, pin.TexCoord);
		float4 normalMap = gNormalMap.Sample(gLinearSample, pin.TexCoord);
		normalMap = normalMap * 2.0f - 1.0f;
		pin.NormalW = mul(normalMap.xyz, TBN);
	}

	
	//float4 diffuse = gMaterialMap[matData.DiffuseMapIndex].Sample(gLinearSample, pin.TexCoord);
	float4 diffuse = pow(gDiffuseMap.Sample(gLinearSample, pin.TexCoord), 2.2f);
	float4 ambient = gAmbientLight * diffuse;
	const float shininess = 1.0f - matData.Roughness;
	
	GBuffer gbuffer;
	gbuffer.Position = float4(pin.PosW, 1.0f);
	gbuffer.Normal = float4(pin.NormalW, 1.0f);
	gbuffer.Albedo = diffuse;
	gbuffer.FresnelShininess = float4(matData.FresnelR0.xyz, shininess);
	
	return gbuffer;
}

#ifndef NUM_DIR_LIGHTS
	#define NUM_DIR_LIGHTS 1
#endif

#ifndef NUM_POINT_LIGHTS
	#define NUM_POINT_LIGHTS 0
#endif

#ifndef NUM_SPOT_LIGHTS
	#define NUM_SPOT_LIGHTS 0
#endif

#include "inc/Common.hlsli"
#include "inc/LightingUtils.hlsli"

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
	float3 PosW : POSITION;
    float3 NormalW : NORMAL;
	float2 TexCoord : TEXCOORD;
	float3 TangentW : TANGENT;
	float3 BitangentW: Binormal;
};

VertexOut VS(VertexIn vin, float4 Color : COLOR)
{
	VertexOut vout;
	
	float4 posW = mul(gWorld, float4(vin.PosL, 1.0f));
	vout.PosW = posW.xyz;
	
	// Transform to homogeneous clip space.
	vout.PosH = mul(gViewProj, posW);
	
	vout.NormalW = mul((float3x3)gWorld, vin.Normal);
	vout.TangentW = mul((float3x3)gWorld, vin.Tangent);
	vout.BitangentW = cross(vout.NormalW, vout.TangentW);
	vout.TexCoord = vin.Tex0;
    return vout;
}

float4 PS(VertexOut pin) : SV_Target
{
	MaterialData matData = gMaterialData[gMaterialIndex];

    pin.NormalW = normalize(pin.NormalW);

	if (matData.hasNormalMap)
	{
		float3 N = pin.NormalW;
		float3 T = normalize(pin.TangentW - dot(pin.TangentW, N) * N);
		float3 B = normalize(pin.BitangentW - dot(pin.BitangentW, N) * N);
		
		float3x3 TBN = { T, B, N };
		float4 normalMap = gMaterialMap[matData.DiffuseMapIndex + 1].Sample(gLinearSample, pin.TexCoord);
		normalMap = normalMap * 2.0f - 1.0f;
		pin.NormalW = mul(normalMap.xyz, TBN);
	}


	float3 viewDir = normalize(gEyePosW - pin.PosW);
	float4 diffuse = gMaterialMap[matData.DiffuseMapIndex].Sample(gLinearSample, pin.TexCoord);
	float4 ambient = gAmbientLight * diffuse;
	const float shininess = 1.0f - matData.Roughness;
	Material mat = { diffuse, matData.FresnelR0, shininess};
	
	float3 shadowFactor = 1.0f;
	
	float4 directLight = ComputeLighting(gLights, mat, pin.PosW, pin.NormalW, viewDir, shadowFactor);
	float4 litColor = directLight;//(ambient + directLight);
	
	litColor.a = matData.DiffuseAlbedo.a;
	
	return litColor;
}
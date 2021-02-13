#define MaxLights 16

struct Light
{
	float3 Strength;
	float FalloffStart;
	float3 Direction;
	float FalloffEnd;
	float3 Position;
	float SpotPower;
	float4x4 ShadowViewProj;
	float4x4 ShadowTransform;
};

struct MaterialData
{
	float4 DiffuseAlbedo;
	float3 FresnelR0;
	float Roughness;
	uint DiffuseMapIndex;
	uint MatPad0;
	uint MatPad1;
	int hasNormalMap;
};

struct Material
{
	float4 DiffuseAlbedo;
	float3 FresnelR0;
	float Shininess;
};

cbuffer cbPerObject : register(b0)
{
	float4x4 gWorld;
	uint gMaterialIndex;
	uint pad1;
	uint pad2;
	uint pad3;
};

cbuffer cbPerPass : register(b1)
{
	float4x4 gView;
	float4x4 gInvView;
	float4x4 gProj;
	float4x4 gInvProj;
	float4x4 gViewProj;
	float4x4 gInvViewProj;
	float3 gEyePosW;
	float cbPerObjectPad1;
	float2 gRenderTargetSize;
	float2 gInvRenderTargetSize;
	float gNearZ;
	float gFarZ;
	float gTotalTime;
	float gDeltaTime;
	float4 gAmbientLight;
	Light gLights[MaxLights];
};

TextureCube gCubeMap : register(t0);
Texture2D gShadowMap : register(t1);

Texture2D gDiffuseMap : register(t2);
Texture2D gNormalMap : register(t3);

StructuredBuffer<MaterialData> gMaterialData : register(t0, space1);
SamplerState gLinearSample : register(s0);
SamplerComparisonState gShadowSample : register(s1);
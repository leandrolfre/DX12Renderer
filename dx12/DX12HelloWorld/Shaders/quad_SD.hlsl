#ifndef NUM_DIR_LIGHTS
#define NUM_DIR_LIGHTS 1
#endif

#ifndef NUM_POINT_LIGHTS
#define NUM_POINT_LIGHTS 0
#endif

#ifndef NUM_SPOT_LIGHTS
#define NUM_SPOT_LIGHTS 0
#endif

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
	float2 TexCoord : TEXCOORD;
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

Texture2D GBufferPosition			: register(t0, space2);
Texture2D GBufferNormal				: register(t1, space2);
Texture2D GBufferAlbedo				: register(t2, space2);
Texture2D GBufferFresnelShininess	: register(t3, space2);

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
	float dx = 1.0f / (2.0f * 1920.0f);
	float dy = 1.0f / (2.0f * 1080.0f);
	
	float4 position = GBufferPosition.Sample(gLinearSample, pin.TexCoord);
	float4 normal = GBufferNormal.Sample(gLinearSample, pin.TexCoord);
	float4 albedo = GBufferAlbedo.Sample(gLinearSample, pin.TexCoord);
	float4 fresnelShininess = GBufferFresnelShininess.Sample(gLinearSample, pin.TexCoord);
	float4 shadowPosH = mul(gLights[0].ShadowTransform,mul(gInvView, float4(position.xyz, 1.0f))); //convert to world position first
	float shadowFactor = CalcShadowFactor(shadowPosH);
	float3 viewDir = normalize(-position.xyz);
	float ao = gSSAOMap.Sample(gLinearSample, pin.TexCoord).r;
	float4 ambient = /*0.1f **/ 0.3 * albedo;

	if (gLights[0].FalloffStart)
	{
		ambient *= ao;
	}
	
	return float4(ao,ao,ao, 1.0);
	float4 directLight = ComputeLighting(gLights, albedo, fresnelShininess.rgb, fresnelShininess.a, position.xyz, normal.xyz, viewDir, shadowFactor);
	
	float4 litColor = (ambient + directLight);

	litColor.a = 1.0f;// albedo.a;
	
	float gamma = 2.2f;
	litColor.rgb = pow(litColor.rgb, float3(1.0f/gamma, 1.0f / gamma, 1.0f / gamma));
	
	return litColor;
}
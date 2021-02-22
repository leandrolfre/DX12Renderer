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

cbuffer cbPerObject : register(b0)
{
	float4x4 gWorld;
	uint gMaterialIndex;
	uint pad1;
	uint pad2;
	uint pad3;
};

cbuffer cbSSAOKernel : register(b1)
{
	float4x4 gProjection;
	float4 gSamples[64];
};

Texture2D GBufferPosition	: register(t0);
Texture2D GBufferNormal		: register(t1);
Texture2D gNoise			: register(t2);
SamplerState gLinearSample : register(s0);
SamplerState gPointSample : register(s1);

VertexOut VS(VertexIn vin, float4 Color : COLOR)
{
	VertexOut vout;
	
	// Transform to homogeneous clip space.
	vout.PosH = float4(vin.PosL, 1.0f);
	vout.TexCoord = vin.Tex0;
    return vout;
}

float2 noiseScale = float2(1920.0f / 4.0f, 1080.0f / 4.0f);
float PS(VertexOut pin) : SV_Target
{
	
	float3 randomVec = gNoise.Sample(gLinearSample, pin.TexCoord * noiseScale).xyz;
	float3 position = GBufferPosition.Sample(gPointSample, pin.TexCoord).xyz;
	float3 normal = normalize(GBufferNormal.Sample(gPointSample, pin.TexCoord).xyz);
	float3 tangent = normalize(randomVec - dot(randomVec, normal) * normal);
	float3 bitangent = cross(normal, tangent);
	float3x3 tbn = {tangent, bitangent, normal};
	
	float occlusion = 0.0f;
	float radius = 0.9f;
	float bias = 0.05f;
	for (int i = 0; i < 64; ++i)
	{
		float3 samplePos = mul(gSamples[i].xyz, tbn );
		samplePos = position + samplePos * radius;
		
		float4 offset = mul(gProjection, float4(samplePos, 1.0f));
		offset.xyz /= offset.w;
		offset.xy = clamp(offset.xy *0.5f + 0.5f, 0.0f, 1.0f);
		offset.y = 1.0f - offset.y;
		
		float sampleDepth = GBufferPosition.Sample(gPointSample, offset.xy).z;
		float rangeCheck = smoothstep(0.0f, 1.0f, radius/abs(position.z - sampleDepth));
		occlusion += (sampleDepth <= samplePos.z - bias ? 1.0f : 0.0f) * rangeCheck;
	}
	occlusion = 1.0f - (occlusion / 64.0f);
	return occlusion;
}
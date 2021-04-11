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


Texture2D GSSAOMap	: register(t0);
SamplerState gLinearSample : register(s0);

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

	float texWidth, texHeight;
	GSSAOMap.GetDimensions(texWidth, texHeight);
	float2 texelSize = float2(1.0f / texWidth, 1.0f / texHeight);
	float result = 0.0;
	for (int x = -2; x < 2; ++x)
	{
		for (int y = -2; y < 2; ++y)
		{
			float2 offset = float2(float(x), float(y)) * texelSize;
			result += GSSAOMap.Sample(gLinearSample, pin.TexCoord + offset).r;
		}
	}
	return (result / (4.0f * 4.0f));
}
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

TextureCube gCubeMap : register(t0);
Texture2D gShadowMap : register(t1);
Texture2D gSSAOMap : register(t2);
SamplerState gLinearSample : register(s0);
SamplerComparisonState gShadowSample : register(s1);

float CalcAttenuation(float d, float falloffStart, float falloffEnd)
{
	return saturate((falloffEnd - d) / (falloffEnd - falloffStart));
}

float3 SchlickFresnel(float3 R0, float3 normal, float3 lightDir)
{
	float cosIncidentAngle = saturate(dot(normal, lightDir));
	float f0 = 1.0f - cosIncidentAngle;
	float3 reflectPercent = R0 + (1.0f - R0) * (f0*f0*f0*f0*f0);
	return reflectPercent;
}

float3 BlinnPhong(float3 lightStrength, float3 lightDir, float3 normal, float3 viewDir, float4 DiffuseAlbedo, float3 FresnelR0, float Shininess)
{
	const float m = Shininess * 1024.0f;
	float3 r = reflect(-viewDir, normal);
	float3 reflectColor = pow(gCubeMap.Sample(gLinearSample, r).rgb, 2.2);
	float3 fresnelFactor = SchlickFresnel(FresnelR0, normal, r);
	float3 halfVec = normalize(viewDir+lightDir);
	float specStrength = 0.8f;
	float3 specular = pow(max(dot(normal, halfVec), 0.0f), m) * specStrength * lightStrength;
	//return (lightStrength * mat.DiffuseAlbedo.rgb + specular);
	return lerp(lightStrength * DiffuseAlbedo.rgb, reflectColor, FresnelR0) + specular;
	
	/*float roughnessFactor = (m + 8.0f) * pow(max(dot(halfVec, normal), 0.0f), m) / 8.0f;
	float3 fresnelFactor = SchlickFresnel(mat.FresnelR0, normal, lightDir);
	float3 specAlbedo = fresnelFactor * roughnessFactor;
	specAlbedo = specAlbedo / (specAlbedo + 1.0f);
	
	return (mat.DiffuseAlbedo.rgb + specAlbedo) * lightStrength;*/
}

float3 ComputeDirectionalLight(Light light, float4 DiffuseAlbedo, float3 FresnelR0, float Shininess, float3 normal, float3 viewDir)
{
	float3 lightDir = normalize(light.Direction);
	float3 lightStrength = light.Strength * max(dot(lightDir, normal), 0.0f);
	
	return BlinnPhong(lightStrength, lightDir, normal, viewDir, DiffuseAlbedo, FresnelR0, Shininess);
}

float3 ComputePointLight(Light light, float4 DiffuseAlbedo, float3 FresnelR0, float Shininess, float3 pos, float3 normal, float3 viewDir)
{
	float3 lightDir = light.Position - pos;
	float distance = length(lightDir);
	
	if (distance > light.FalloffEnd) return 0.0f;
	
	//normalize
	lightDir /= distance;
	
	float3 lightStrength = light.Strength * max(dot(lightDir, normal), 0.0f);
	
	float attenuation = CalcAttenuation(distance, light.FalloffStart, light.FalloffEnd);
	
	lightStrength *= attenuation;
	
	return BlinnPhong(lightStrength, lightDir, normal, viewDir, DiffuseAlbedo, FresnelR0, Shininess);
}

float3 ComputeSpotLight(Light light, float4 DiffuseAlbedo, float3 FresnelR0, float Shininess, float3 pos, float3 normal, float3 viewDir)
{
	float3 lightDir = light.Position - pos;
	float distance = length(lightDir);
	
	if (distance > light.FalloffEnd) return 0.0f;
	
	//normalize
	lightDir /= distance;
	
	float3 lightStrength = light.Strength * max(dot(lightDir, normal), 0.0f);
	
	float attenuation = CalcAttenuation(distance, light.FalloffStart, light.FalloffEnd);
	
	lightStrength *= attenuation;
	
	float spotFactor = pow(max(dot(-lightDir, light.Direction), 0.0f), light.SpotPower);
	lightStrength *= spotFactor;
	
	return BlinnPhong(lightStrength, lightDir, normal, viewDir, DiffuseAlbedo, FresnelR0, Shininess);
}

float4 ComputeLighting(Light gLights[MaxLights], float4 DiffuseAlbedo, float3 FresnelR0, float Shininess, float3 pos, float3 normal, float3 viewDir, float shadowFactor)
{
	float3 result = 0.0f;
	int i = 0;
	#if (NUM_DIR_LIGHTS > 0)
		for(i = 0; i < NUM_DIR_LIGHTS; ++i)
		{
			result += shadowFactor * ComputeDirectionalLight(gLights[i], DiffuseAlbedo, FresnelR0, Shininess, normal, viewDir);
		}
	#endif
	
	#if (NUM_POINT_LIGHTS > 0)
		for(i = NUM_DIR_LIGHTS; i < NUM_DIR_LIGHTS+NUM_POINT_LIGHTS; ++i)
		{
			result += ComputePointLight(gLights[i], DiffuseAlbedo, FresnelR0, Shininess, pos, normal, viewDir);
		}
	#endif
	
	#if (NUM_SPOT_LIGHTS > 0)
		for(i = NUM_DIR_LIGHTS+NUM_POINT_LIGHTS; i < NUM_DIR_LIGHTS+NUM_POINT_LIGHTS+NUM_SPOT_LIGHTS; ++i)
		{
			result += ComputeSpotLight(gLights[i], DiffuseAlbedo, FresnelR0, Shininess, pos, normal, viewDir);
		}
	#endif
	
	return float4(result, 0.0f);
}

float CalcShadowFactor(float4 shadowPosH)
{
	shadowPosH.xyz / shadowPosH.w;

	float depth = shadowPosH.z;
	uint width, height, numMips;
	gShadowMap.GetDimensions(0, width, height, numMips);

	float dx = 1.0f / (float)width;
	float dy = 1.0f / (float)height;
	const int offsetSampleNum = 9;
	const float2 offsets[offsetSampleNum] = {
		float2(-dx, -dy), float2(0.0f, -dy), float2(dx, -dy),
		float2(-dx, 0.0f), float2(0.0f, 0.0f), float2(dx, 0.0f),
		float2(-dx, dy), float2(0.0f, dy), float2(dx, dy)
	};

	float pcf = 0.0f;
	[unroll]
	for (int i = 0; i < offsetSampleNum; ++i)
	{
		pcf += gShadowMap.SampleCmpLevelZero(gShadowSample, shadowPosH.xy + offsets[i], depth).r;
	}
	return pcf / (float)offsetSampleNum;
}
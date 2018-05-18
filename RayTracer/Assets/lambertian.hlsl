#include "illum.hs"
#include "psinput.hs"
#include "std_cbuffer.h"

ConstantBuffer<IllumGlobalConstants> g_illumGlobalConstants : register(b0, space1);
ConstantBuffer<LightSourceConstants> g_lightSourceConstants[] : register(b1, space1);

Texture2D g_albedoTexture : register(t0);
SamplerState g_albedoSampler : register(s0);

float4 PSMain(PSInput input) : SV_TARGET
{	
	float3 ambientRgb = g_illumGlobalConstants.ambientIntensity.rgb;
	int lightSourceCount = g_illumGlobalConstants.lightSourceCount.x;
	// to fix the value after iterpolation
	const float3 vN = normalize(input.normalV);

	float4 diffCol = g_albedoTexture.Sample(g_albedoSampler, input.uv);
	float3 vLightInts = float3(0.0f, 0.0f, 0.0f);

	for (int i = 0; i < lightSourceCount; ++i)
	{
		float3 vL = g_lightSourceConstants[i].lightPositionView.xyz - input.positionV;
		float3 lightCol = g_lightSourceConstants[0].lightIntensity.rgb;
		float d = length(vL); vL = normalize(vL);
		float4 lightAtten = g_lightSourceConstants[i].lightAttenuation;
		float attenuation = saturate(1.0f / (lightAtten.x + lightAtten.y * d + lightAtten.z * d * d) - lightAtten.w);

		// diffuse only
		const float fDiff = saturate(dot(vL, vN));
		vLightInts += attenuation * lightCol * diffCol.rgb * fDiff;
	}

	vLightInts += (diffCol.rgb * ambientRgb);

	// the gamma correction, to the approximation, use the power 1/gamma, and the gamma == 2, which is just square-root.
	vLightInts = sqrt(vLightInts);
	
	//float2 rg = input.uv;
	//rg.r %= 1.0f;
	//rg.g %= 1.0f;
	//return float4(rg, 0.0f, 1.0f);
	return float4(vLightInts, diffCol.a);
}
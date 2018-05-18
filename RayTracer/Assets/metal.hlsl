#include "illum.hs"
#include "psinput.hs"
#include "std_cbuffer.h"

ConstantBuffer<MetalConstants> g_mtlConstants : register(b1);

ConstantBuffer<IllumGlobalConstants> g_illumGlobalConstants : register(b0, space1);
ConstantBuffer<LightSourceConstants> g_lightSourceConstants[] : register(b1, space1);

Texture2D g_albedoTexture : register(t0);
SamplerState g_albedoSampler : register(s0);

float4 PSMain(PSInput input) : SV_TARGET
{
	float specPow = 32; // hardcode the specular power for simplified modeling 
	float n = (1.0f - g_mtlConstants.fuzziness.x) * specPow;

	float3 ambientRgb = g_illumGlobalConstants.ambientIntensity.rgb;
	int lightSourceCount = g_illumGlobalConstants.lightSourceCount.x;
	// to fix the value after iterpolation
	const float3 vN = normalize(input.normalV);
	const float3 vT = normalize(input.tangentV.xyz - dot(input.tangentV.xyz, vN) * vN);
	const float3 vB = input.tangentV.w * cross(vN, vT);

	float4 diffCol = g_albedoTexture.Sample(g_albedoSampler, input.uv);
	float3 vLightInts = float3(0.0f, 0.0f, 0.0f);
	const float3 vV = normalize(float3(0, 0, 0) - input.positionV);

	for (int i = 0; i < lightSourceCount; ++i)
	{
		float3 vL = g_lightSourceConstants[i].lightPositionView.xyz - input.positionV;
		float3 lightCol = g_lightSourceConstants[0].lightIntensity.rgb;
		float d = length(vL); vL = normalize(vL);
		float4 lightAtten = g_lightSourceConstants[i].lightAttenuation;
		float attenuation = saturate(1.0f / (lightAtten.x + lightAtten.y * d + lightAtten.z * d * d) - lightAtten.w);

		// diffuse only
		vLightInts += attenuation * lightCol * BRDF_ts_nphong_nofr(vN, vL, vV, diffCol.rgb, lightCol, n);
	}

	vLightInts += (diffCol.rgb * ambientRgb);

	// the gamma correction, to the approximation, use the power 1/gamma, and the gamma == 2, which is just square-root.
	vLightInts = sqrt(vLightInts);

	return float4(vLightInts, diffCol.a);
}
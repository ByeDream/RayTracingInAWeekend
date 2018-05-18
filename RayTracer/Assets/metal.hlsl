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
	float specPow = 16; // hardcode the specular power for simplified modeling 

	// to fix the value after iterpolation
	const float3 vN = normalize(input.normalV);
	const float3 vT = normalize(input.tangentV.xyz - dot(input.tangentV.xyz, vN) * vN);
	const float3 vB = input.tangentV.w * cross(vN, vT);
	float3 vL = g_lightSourceConstants[0].lightPositionView.xyz - input.positionV;
	vL = normalize(vL);
	const float3 vV = normalize(float3(0, 0, 0) - input.positionV);

	float4 diff = g_albedoTexture.Sample(g_albedoSampler, input.uv);

	float n = (1.0f - g_mtlConstants.fuzziness.x) * specPow;
	float3 vLightInts = g_lightSourceConstants[0].lightIntensity.rgb * BRDF_ts_nphong_nofr(vN, vL, vV, diff.rgb, diff.rgb, n);
	vLightInts += (diff.rgb * g_illumGlobalConstants.ambientIntensity.rgb);

	// the gamma correction, to the approximation, use the power 1/gamma, and the gamma == 2, which is just square-root.
	vLightInts = sqrt(vLightInts);

	return float4(vLightInts, diff.a);
}
#include "illum.hs"

struct PSInput
{
	float4 position		: SV_POSITION;
	float3 normalV		: TEXCOORD0;
	float4 tangentV		: TEXCOORD1;
	float3 positionV	: TEXCOORD2;
	float2 uv			: TEXCOORD3;
};

cbuffer MaterialConstants : register(b1)
{
	float4 g_placeholder;
};

cbuffer IllumConstants : register(b2)
{
	float4 g_lightDirV;				// Light direction in view space.
	float4 g_lightIntensity;
	float4 g_ambientIntensity;
};

Texture2D g_albedoTexture : register(t0);
SamplerState g_albedoSampler : register(s0);

float4 PSMain(PSInput input) : SV_TARGET
{						
	// to fix the value after iterpolation
	const float3 vN = normalize(input.normalV);
	const float3 vL = normalize(g_lightDirV.xyz);

	// diffuse only
	const float fDiff = saturate(dot(vL, vN));
	float4 diff = g_albedoTexture.Sample(g_albedoSampler, input.uv);
	float3 vLightInts = g_lightIntensity.rgb * diff.rgb * fDiff;
	vLightInts += (diff.rgb * g_ambientIntensity);

	// the gamma correction, to the approximation, use the power 1/gamma, and the gamma == 2, which is just square-root.
	vLightInts = sqrt(vLightInts);

	return float4(vLightInts, diff.a);
}
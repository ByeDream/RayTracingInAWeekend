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
	float4 g_albedo;
};

cbuffer IllumConstants : register(b2)
{
	float4 g_lightDirV;				// Light direction in view space.
	float4 g_lightIntensity;
	float4 g_ambientIntensity;
};

float4 PSMain(PSInput input) : SV_TARGET
{						
	// to fix the value after iterpolation
	const float3 vN = normalize(input.normalV);
	const float3 vL = normalize(g_lightDirV);

	// diffuse only
	const float fDiff = saturate(dot(vL, vN));
	float3 vLightInts = g_lightIntensity * g_albedo.rgb * fDiff;
	vLightInts += (g_albedo.rgb * g_ambientIntensity);

	// the gamma correction, to the approximation, use the power 1/gamma, and the gamma == 2, which is just square-root.
	vLightInts = sqrt(vLightInts);

	return float4(vLightInts, 1.0f);
}
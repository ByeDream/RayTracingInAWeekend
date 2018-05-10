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
	float4 g_refractiveIndex;
};

cbuffer IllumConstants : register(b2)
{
	float4 g_lightDirV;				// Light direction in view space.
	float4 g_lightIntensity;
	float4 g_ambientIntensity;
};

float4 PSMain(PSInput input) : SV_TARGET
{
	float specPow = 3; // hardcode the specular power for simplified modeling 
	float3 diff_c = float3(1.0f, 1.0f, 1.0f);

						// to fix the value after iterpolation
	const float3 vN = normalize(input.normalV);
	const float3 vT = normalize(input.tangentV.xyz - dot(input.tangentV.xyz, vN) * vN);
	const float3 vB = input.tangentV.w * cross(vN, vT);
	const float3 vL = normalize(g_lightDirV);
	const float3 vV = normalize(float3(0, 0, 0) - input.positionV);

	float3 vLightInts = g_lightIntensity * BRDF_ts_nphong_nofr(vN, vL, vV, diff_c, diff_c, specPow);
	vLightInts += (diff_c * g_ambientIntensity);

	// the gamma correction, to the approximation, use the power 1/gamma, and the gamma == 2, which is just square-root.
	vLightInts = sqrt(vLightInts);

	return float4(vLightInts, 1.0f);
}
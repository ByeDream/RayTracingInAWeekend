#include "illum.hs"

struct PSInput
{
	float4 position		: SV_POSITION;
	float3 normalV		: TEXCOORD0;
	float4 tangentV		: TEXCOORD1;
	float3 positionV	: TEXCOORD2;
	float2 uv			: TEXCOORD3;
};

struct IllumGlobalConstants
{
	float4	ambientIntensity;
	float4	lightSourceCount;
};

struct LightSourceConstants
{
	float4	lightPositionView;
	float4	lightIntensity;
	float4  lightAttenuation;
};

ConstantBuffer<IllumGlobalConstants> g_illumGlobalConstants : register(b0, space1);
ConstantBuffer<LightSourceConstants> g_lightSourceConstants[] : register(b1, space1);

Texture2D g_albedoTexture : register(t0);
SamplerState g_albedoSampler : register(s0);

float4 PSMain(PSInput input) : SV_TARGET
{						
	// to fix the value after iterpolation
	const float3 vN = normalize(input.normalV);

	float3 vL = g_lightSourceConstants[0].lightPositionView.xyz - input.positionV;
	vL = normalize(vL);

	// diffuse only
	const float fDiff = saturate(dot(vL, vN));
	float4 diff = g_albedoTexture.Sample(g_albedoSampler, input.uv);
	float3 vLightInts = g_lightSourceConstants[0].lightIntensity.rgb * diff.rgb * fDiff;
	vLightInts += (diff.rgb * g_illumGlobalConstants.ambientIntensity.rgb);

	// the gamma correction, to the approximation, use the power 1/gamma, and the gamma == 2, which is just square-root.
	vLightInts = sqrt(vLightInts);
	
	//float2 rg = input.uv;
	//rg.r %= 1.0f;
	//rg.g %= 1.0f;
	//return float4(rg, 0.0f, 1.0f);
	return float4(vLightInts, diff.a);
}
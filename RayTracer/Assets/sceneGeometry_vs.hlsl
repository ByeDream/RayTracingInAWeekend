#include "psinput.hs"
#include "std_cbuffer.h"

struct VSInput
{
	float3 position		: POSITION;
	float3 normal		: NORMAL;
	float4 tangent		: TANGENT;
	float2 uv			: TEXCOORD0;
};

ConstantBuffer<GeometryConstants> g_geoConstants : register(b0);

PSInput VSMain(VSInput input)
{
	PSInput result;

	result.position = mul(float4(input.position, 1.0f), g_geoConstants.worldViewProj);
	result.positionV = mul(float4(input.position.xyz, 1.0f), g_geoConstants.worldView).xyz;
	result.normalV = mul(float4(input.normal, 0.0f), g_geoConstants.worldView).xyz;
	result.tangentV = float4(mul(float4(input.tangent.xyz, 0), g_geoConstants.worldView).xyz, input.tangent.w);

	result.uv = input.uv;

	return result;
}

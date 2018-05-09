struct VSInput
{
	float3 position		: POSITION;
	float3 normal		: NORMAL;
	float4 tangent		: TANGENT;
	float2 uv			: TEXCOORD0;
};

struct PSInput
{
	float4 position		: SV_POSITION;
	float3 normalV		: TEXCOORD0;
	float4 tangentV		: TEXCOORD1;
	float3 positionV	: TEXCOORD2;
	float2 uv			: TEXCOORD3;
};

cbuffer GeometryConstants : register(b0)
{
	float4x4 g_mWorldViewProj;
	float4x4 g_mWorldView;
};

PSInput VSMain(VSInput input)
{
	PSInput result;

	result.position = mul(float4(input.position, 1.0f), g_mWorldViewProj);
	result.positionV = mul(float4(input.position.xyz, 1.0f), g_mWorldView).xyz;
	result.normalV = mul(float4(input.normal, 0.0f), g_mWorldView).xyz;
	result.tangentV = float4(mul(float4(input.tangent.xyz, 0), g_mWorldView).xyz, input.tangent.w);

	result.uv = input.uv;

	return result;
}

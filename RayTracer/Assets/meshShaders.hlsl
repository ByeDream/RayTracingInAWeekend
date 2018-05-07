struct VSInput
{
	float3 position	: POSITION;
	float3 normal	: NORMAL;
	float4 tangent	: TANGENT;
	float2 uv		: TEXCOORD0;
};

struct PSInput
{
	float4 position : SV_POSITION;
	float2 uv : TEXCOORD;
};

cbuffer cb0 : register(b0)
{
	float4x4 g_mWorldViewProj;
};

PSInput VSMain(VSInput input)
{
	PSInput result;

	result.position = mul(float4(input.position, 1.0f), g_mWorldViewProj);
	result.uv = input.uv;

	return result;
}

float4 PSMain(PSInput input) : SV_TARGET
{
	return float4(1.0f, 0.0f, 0.0f, 1.0f);
}
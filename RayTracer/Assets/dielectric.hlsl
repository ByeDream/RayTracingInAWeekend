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

float4 PSMain(PSInput input) : SV_TARGET
{
	return float4(0.8f, 0.8f, 0.8f, 0.8f);
}
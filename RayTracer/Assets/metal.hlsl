struct PSInput
{
	float4 position : SV_POSITION;
	float2 uv : TEXCOORD;
};

cbuffer PSCBuffer : register(b1)
{
	float4x4 g_mWorldViewProj;
};
float4 PSMain(PSInput input) : SV_TARGET
{
	return float4(0.0f, 0.0f, 1.0f, 1.0f);
}
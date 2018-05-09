struct PSInput
{
	float4 position : SV_POSITION;
	float2 uv : TEXCOORD;
};

cbuffer MaterialConstants : register(b1)
{
	float4 g_albedo;
};
float4 PSMain(PSInput input) : SV_TARGET
{
	return float4(g_albedo.rgb, 1.0f);
}
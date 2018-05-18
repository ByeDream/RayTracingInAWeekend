#include "psinput.hs"
#include "std_cbuffer.h"

ConstantBuffer<DiffuseLightConstants> g_mtlConstants : register(b1);

float4 PSMain(PSInput input) : SV_TARGET
{
	return float4(g_mtlConstants.intensity.rgb, 1.0f);
}
//#include "illum.hs"

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
float4 PSMain(PSInput input) : SV_TARGET
{

	// TODO light
	float3 lightPosition = float3(0.5f, 0.58f, 0.8f);
	float3 lightRgb = 1.3f * float3(0.5f, 0.58f, 0.8f);
	float4 lightAtten = float4(1, 0, 0, 0);

	float3 ambientRgb = 0.3f * float3(0.18f, 0.2f, 0.3f);
	float specPow = 30; // hardcode the specular power for simplified modeling 
						
	// to fix the value after iterpolation
	const float3 vN = normalize(input.normalV);
	const float3 vT = normalize(input.tangentV.xyz - dot(input.tangentV.xyz, vN) * vN);
	const float3 vB = input.tangentV.w * cross(vN, vT);
	
	float3 vL = lightPosition - input.positionV;
	float d = length(vL); vL = normalize(vL);
	const float3 vV = normalize(float3(0, 0, 0) - input.positionV);

	float3 myhalf = normalize(vV + vL);

	float fDiff = saturate(dot(vL, vN));
	float fSpec = pow(saturate(dot(myhalf, vN)), 20);

	float3 res = g_albedo.rgb * fDiff; //+ float3(0.6, 0.6, 0.6) * fSpec;

	return float4(res, 1.0f);
	/*
	float3 vLightInts = lightRgb * BRDF(vN, vL, vV, g_albedo.rgb, float3(0.6, 0.6, 0.6));
	return float4(vLightInts, 1.0f);
	

	float d = length(vL); vL = normalize(vL);
	float attenuation = saturate(1.0f / (lightAtten.x + lightAtten.y * d + lightAtten.z * d * d) - lightAtten.w);
	float3 vLightInts = attenuation * lightRgb*BRDF2_ts_nphong_nofr(vBumpNorm, vGeomNorm, vL, vV, diff_col, spec_col, specPow);
	return float4(vLightInts, 1.0f);
	*/







	////////////////////////////////////////

	/*
	float3 lightRgb = m_lightColor.xyz;
	float4 lightAtten = m_lightAttenuation;
	float3 ambientRgb = m_ambientColor.xyz;
	float specPow = 30; // hardcode the specular power for simplified modeling 

						// to fix the value after iterpolation
						//const float3 vN = In.vNorm;
	const float3 vN = normalize(In.vNorm);
	//const float3 vT = In.vTang.xyz;
	const float3 vT = normalize(In.vTang.xyz - dot(In.vTang.xyz, vN) * vN);

	const float3 vB = In.vTang.w * cross(vN, vT); // why multiply by vT.w? I can't understand
	float3 vL = m_lightPosition.xyz - In.vPosInView;
	const float3 vV = normalize(float3(0, 0, 0) - In.vPosInView);
	float d = length(vL); vL = normalize(vL);
	float attenuation = saturate(1.0f / (lightAtten.x + lightAtten.y * d + lightAtten.z * d * d) - lightAtten.w);

	float4 normalGloss = bumpGlossMap.Sample(samp0, In.TextureUV.xy);
	// Uncompress each component from [0,1] to [1,1]
	normalGloss.xyz = normalGloss.xyz * 2.0f - 1.0f;
	normalGloss.y = -normalGloss.y;		// normal map has green channel inverted

	float3 vBumpNorm = normalize(normalGloss.x*vT + normalGloss.y*vB + normalGloss.z*vN);
	//float3 vBumpNorm = normalize(vN); //@@LRF, use bump normal as geometry normal at the moment as we don't have normal mapping yet.
	float3 vGeomNorm = normalize(vN);


	float3 diff_col = colorMap.Sample(samp0, In.TextureUV.xy).xyz;
	//	float3 diff_col = In.Color.rgb; // use vertex color as meterial diffuse color at the moment before I add texturing.

	float4 specGloss = specGlossMap.Sample(samp0, In.TextureUV.xy);
	float3 spec_col = 0.45*specGloss.x + 0.05;
	//float3 spec_col = float3(0.5f, 0.5f, 0.5f); // hard code the meterial specular color at the moment as well.

	float3 vLightInts = attenuation * lightRgb*BRDF2_ts_nphong_nofr(vBumpNorm, vGeomNorm, vL, vV, diff_col, spec_col, specPow);

	float3 ambient = float3(0.0f, 0.0f, 0.0f);
	ambient = diff_col * ambientRgb;  // use meterial ambient as meterial diffuse for simple
	vLightInts += ambient;

	//if (result.color.a < 0.5f)
	//	discard;
	return float4(vLightInts, In.Color.a);
	*/
}
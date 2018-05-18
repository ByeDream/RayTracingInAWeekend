#ifndef __STDCBUFFER_H__
#define __STDCBUFFER_H__

#ifndef __cplusplus

#define Vector2Unaligned		float2
#define Vector3Unaligned		float3
#define Vector4Unaligned		float4
#define Matrix4Unaligned		float4x4

#else

#define Vector2Unaligned		XMFLOAT2
#define Vector3Unaligned		XMFLOAT3
#define Vector4Unaligned		XMFLOAT4
#define Matrix4Unaligned		XMFLOAT4X4

#endif

struct GeometryConstants
{
	Matrix4Unaligned	worldViewProj;
	Matrix4Unaligned	worldView;
};

struct IllumGlobalConstants
{
	Vector4Unaligned	ambientIntensity;
	Vector4Unaligned	lightSourceCount;
};

struct LightSourceConstants
{
	Vector4Unaligned	lightPositionView;
	Vector4Unaligned	lightIntensity;
	Vector4Unaligned	lightAttenuation;
};

struct DiffuseLightConstants
{
	Vector4Unaligned	intensity;
};

struct MetalConstants
{
	Vector4Unaligned	fuzziness;
};

struct DielectricConstants
{
	Vector4Unaligned	refractiveIndex;
};

#endif // __STDCBUFFER_H__
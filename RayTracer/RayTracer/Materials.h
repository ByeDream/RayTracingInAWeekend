#pragma once

#include "Vec3.h"
#include "D3D12Defines.h"
#include "std_cbuffer.h"

enum MaterialID
{
	MID_DIFFUSE_LIGHT = 0,
	MID_LAMBERTIAN,
	MID_METAL,
	MID_DIELECTRIC,

	MID_COUNT
};

class Ray;
struct HitRecord;
class D3D12Viewer;
class ITexture2D;

struct MaterialD3D12Resources
{
	ComPtr<ID3D12Resource>			m_MtlConstantBuffer;
	CD3DX12_GPU_DESCRIPTOR_HANDLE	m_MtlCbvHandle;
};

class IMaterial
{
public:
	MaterialD3D12Resources m_d3dRes;

	virtual ~IMaterial() = default;
	virtual BOOL Scatter(const Ray &r_in, const HitRecord &rec, Vec3 &attenuation, Ray &r_scattered, Vec3 &emitted) const = 0;
	virtual MaterialID GetID() const = 0;

	virtual void ApplySRV(D3D12Viewer *viewer) const {}
	virtual void ApplyCBV(D3D12Viewer *viewer, D3D12_GPU_DESCRIPTOR_HANDLE illumCbvHandle) const {}

	virtual void BuildD3DRes(D3D12Viewer *viewer, CD3DX12_CPU_DESCRIPTOR_HANDLE &cbvCPUHandle, CD3DX12_GPU_DESCRIPTOR_HANDLE &cbvGPUHandle);
	virtual size_t GetDataSize() const { return 0; }
	virtual const void * GetDataPtr() const { return nullptr; }
};

class Lambertian : public IMaterial
{
public:
	const ITexture2D *m_albedo{ nullptr }; // the reflectance

	Lambertian(const ITexture2D *albedo);
	virtual BOOL Scatter(const Ray &r_in, const HitRecord &rec, Vec3 &attenuation, Ray &r_scattered, Vec3 &emitted) const override;
	virtual MaterialID GetID() const override { return Lambertian::GetStaticID(); }

	virtual void ApplySRV(D3D12Viewer *viewer) const override;
	virtual void ApplyCBV(D3D12Viewer *viewer, D3D12_GPU_DESCRIPTOR_HANDLE illumCbvHandle) const override;

	static PipelineState s_pso;
	static void BuildPSO(D3D12Viewer *viewer, UINT32 lightSourceCount);
	static void ApplyPSO(D3D12Viewer *viewer);
	static MaterialID GetStaticID() { return MID_LAMBERTIAN; }
};

class Metal : public IMaterial
{
public:
	MetalConstants m_data;
	ITexture2D *m_albedo{ nullptr }; // the reflectance

	Metal(ITexture2D *albedo, float fuzziness);
	
	virtual BOOL Scatter(const Ray &r_in, const HitRecord &rec, Vec3 &attenuation, Ray &r_scattered, Vec3 &emitted) const override;
	virtual MaterialID GetID() const override { return Metal::GetStaticID(); }

	virtual void ApplySRV(D3D12Viewer *viewer) const override;
	virtual void ApplyCBV(D3D12Viewer *viewer, D3D12_GPU_DESCRIPTOR_HANDLE illumCbvHandle) const override;

	virtual size_t GetDataSize() const override { return sizeof(m_data); }
	virtual const void * GetDataPtr() const override { return &m_data; }

	static PipelineState s_pso;
	static void BuildPSO(D3D12Viewer *viewer, UINT32 lightSourceCount);
	static void ApplyPSO(D3D12Viewer *viewer);
	static MaterialID GetStaticID() { return MID_METAL; }
};

class Dielectric : public IMaterial
{
public:
	DielectricConstants m_data;

	Dielectric(float refractiveIndex); 
	virtual BOOL Scatter(const Ray &r_in, const HitRecord &rec, Vec3 &attenuation, Ray &r_scattered, Vec3 &emitted) const override;
	virtual MaterialID GetID() const override { return Dielectric::GetStaticID(); }

	virtual void ApplyCBV(D3D12Viewer *viewer, D3D12_GPU_DESCRIPTOR_HANDLE illumCbvHandle) const override;

	virtual size_t GetDataSize() const override { return sizeof(m_data); }
	virtual const void * GetDataPtr() const override { return &m_data; }

	static PipelineState s_pso;
	static void BuildPSO(D3D12Viewer *viewer, UINT32 lightSourceCount);
	static void ApplyPSO(D3D12Viewer *viewer);
	static MaterialID GetStaticID() { return MID_DIELECTRIC; }
};

class DiffuseLight : public IMaterial
{
public:
	DiffuseLightConstants m_data;

	DiffuseLight(const Vec3 intensity);
	virtual BOOL Scatter(const Ray &r_in, const HitRecord &rec, Vec3 &attenuation, Ray &r_scattered, Vec3 &emitted) const override;
	virtual MaterialID GetID() const override { return DiffuseLight::GetStaticID(); }

	virtual void ApplyCBV(D3D12Viewer *viewer, D3D12_GPU_DESCRIPTOR_HANDLE illumCbvHandle) const override;

	virtual size_t GetDataSize() const override { return sizeof(m_data); }
	virtual const void * GetDataPtr() const override { return &m_data; }

	static PipelineState s_pso;
	static void BuildPSO(D3D12Viewer *viewer);
	static void ApplyPSO(D3D12Viewer *viewer);
	static MaterialID GetStaticID() { return MID_DIFFUSE_LIGHT; }
};
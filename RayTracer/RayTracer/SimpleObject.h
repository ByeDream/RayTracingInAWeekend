#pragma once

#include "Vec3.h"

class Mesh;
class IHitable;
class IMaterial;
class D3D12Viewer;
class SimpleCamera;

struct GeometryConstants
{
	XMFLOAT4X4 mvp;		// Model-view-projection (MVP) matrix.
	FLOAT padding[48];
};

struct ObjectD3D12Resources
{
	GeometryConstants *				m_pGeoConstants;
	ComPtr<ID3D12Resource>			m_GeoConstantBuffer;
	UINT32							m_GeoConstantBufferSize;
	CD3DX12_GPU_DESCRIPTOR_HANDLE *	m_GeoCbvHandles;

	UINT8 *							m_pMtlConstants;
	ComPtr<ID3D12Resource>			m_MtlConstantBuffer;
	UINT32							m_MtlConstantBufferSize;
	CD3DX12_GPU_DESCRIPTOR_HANDLE *	m_MtlCbvHandles;


	ComPtr<ID3D12DescriptorHeap>	m_CbvHeap;
	UINT32							m_CurrentCbvIndex;

	~ObjectD3D12Resources() 
	{
		delete[] m_GeoCbvHandles; 
		delete[] m_MtlCbvHandles;
	}
};

class Object
{
public:
	virtual ~Object() = default;
	Vec3						m_position;
	float						m_scale;
	// TODO for rotation

	Mesh *						m_mesh;
	IHitable *					m_hitable;
	IMaterial *					m_material;

	ObjectD3D12Resources		m_d3dRes;

	virtual void				Update(SimpleCamera *camera, float elapsedSeconds) = 0;
	virtual void				Render(D3D12Viewer *viewer) const = 0;
	virtual void				BuildD3DRes(D3D12Viewer *viewer) = 0;
};




class SimpleSphereObject : public Object
{
public:
	SimpleSphereObject(const Vec3 &center, float radius, Mesh *mesh, IMaterial *material);
	virtual ~SimpleSphereObject() override;

	virtual void				Update(SimpleCamera *camera, float elapsedSeconds) override;
	virtual void				Render(D3D12Viewer *viewer) const override;
	virtual void				BuildD3DRes(D3D12Viewer *viewer) override;
};

// TODO more
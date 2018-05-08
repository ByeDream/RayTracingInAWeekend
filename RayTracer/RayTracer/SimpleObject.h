#pragma once

#include "Vec3.h"

class Mesh;
class IHitable;
class IMaterial;
class D3D12Viewer;
class SimpleCamera;

struct VSConstants
{
	XMFLOAT4X4 mvp;		// Model-view-projection (MVP) matrix.
	FLOAT padding[48];
};

struct ObjectD3D12Resources
{
	VSConstants *					m_pVSConstants;
	ComPtr<ID3D12Resource>			m_VSConstantBuffer;
	ComPtr<ID3D12DescriptorHeap>	m_VSCbvHeap;
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

	virtual void				Update(D3D12Viewer *viewer, SimpleCamera *camera, float elapsedSeconds) = 0;
	virtual void				Render(D3D12Viewer *viewer) const = 0;
	virtual void				BuildD3DRes(D3D12Viewer *viewer) = 0;
};




class SimpleSphereObject : public Object
{
public:
	SimpleSphereObject(const Vec3 &center, float radius, Mesh *mesh, IMaterial *material);
	virtual ~SimpleSphereObject();

	virtual void				Update(D3D12Viewer *viewer, SimpleCamera *camera, float elapsedSeconds);
	virtual void				Render(D3D12Viewer *viewer) const;
	virtual void				BuildD3DRes(D3D12Viewer *viewer);
};

// TODO more
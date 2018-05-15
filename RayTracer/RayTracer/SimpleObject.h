#pragma once

#include "Vec3.h"
#include "AABB.h"

class Mesh;
class IHitable;
class IMaterial;
class D3D12Viewer;
class SimpleCamera;
class World;
struct HitRecord;

struct GeometryConstants
{
	XMFLOAT4X4 mWorldViewProj;		// Model-view-projection (MVP) matrix.
	XMFLOAT4X4 mWorldView;			// Model-view (MV) matrix.
};

struct ObjectD3D12Resources
{
	UINT8 *							m_pGeoConstants;
	ComPtr<ID3D12Resource>			m_GeoConstantBuffer;
	UINT32							m_GeoConstantBufferSize;
	CD3DX12_GPU_DESCRIPTOR_HANDLE *	m_GeoCbvHandles;

	UINT8 *							m_pMtlConstants;
	ComPtr<ID3D12Resource>			m_MtlConstantBuffer;
	UINT32							m_MtlConstantBufferSize;
	CD3DX12_GPU_DESCRIPTOR_HANDLE *	m_MtlCbvHandles;

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
	World *						m_world;

	Vec3						m_position;
	float						m_scale;
	// TODO for rotation

	Mesh *						m_mesh;
	IHitable *					m_hitable;
	IMaterial *					m_material;

	ObjectD3D12Resources		m_d3dRes;

	virtual void				Update(SimpleCamera *camera, float elapsedSeconds) = 0;
	virtual void				Render(D3D12Viewer *viewer, UINT32 mid) const = 0;
	virtual AABB				BoundingBox() const = 0; // return bool as some object doesn't have bounding-box like a plane
	virtual BOOL				Hit(const Ray &r, float &t_min, float &t_max, HitRecord &out_rec) const  = 0;
	virtual void				BuildD3DRes(D3D12Viewer *viewer, CD3DX12_CPU_DESCRIPTOR_HANDLE &cbvCPUHandle, CD3DX12_GPU_DESCRIPTOR_HANDLE &cbvGPUHandle) = 0;
};




class SimpleObjectSphere : public Object
{
public:
	SimpleObjectSphere(const Vec3 &center, float radius, Mesh *mesh, IMaterial *material, World *world);
	virtual ~SimpleObjectSphere() override;

	virtual void				Update(SimpleCamera *camera, float elapsedSeconds) override;
	virtual void				Render(D3D12Viewer *viewer, UINT32 mid) const override;
	virtual AABB				BoundingBox() const override;
	virtual BOOL				Hit(const Ray &r, float &t_min, float &t_max, HitRecord &out_rec) const override;
	virtual void				BuildD3DRes(D3D12Viewer *viewer, CD3DX12_CPU_DESCRIPTOR_HANDLE &cbvCPUHandle, CD3DX12_GPU_DESCRIPTOR_HANDLE &cbvGPUHandle) override;
};

// TODO more

class SimpleObjectBVHNode : public Object
{
public:
	SimpleObjectBVHNode(std::vector<Object *> objects);
	virtual ~SimpleObjectBVHNode() override;

	virtual void				Update(SimpleCamera *camera, float elapsedSeconds) override;
	virtual void				Render(D3D12Viewer *viewer, UINT32 mid) const override;
	virtual BOOL				Hit(const Ray &r, float &t_min, float &t_max, HitRecord &out_rec) const override;
	virtual AABB				BoundingBox() const override;
	virtual void				BuildD3DRes(D3D12Viewer *viewer, CD3DX12_CPU_DESCRIPTOR_HANDLE &cbvCPUHandle, CD3DX12_GPU_DESCRIPTOR_HANDLE &cbvGPUHandle) override;

	AABB m_bindingBox;
	Object *leftChild{ nullptr };
	Object *rightChild{ nullptr };
};
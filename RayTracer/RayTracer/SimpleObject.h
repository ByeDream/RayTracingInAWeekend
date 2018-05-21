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

struct ObjectD3D12Resources
{
	UINT8 *							m_pGeoConstants;
	ComPtr<ID3D12Resource>			m_GeoConstantBuffer;
	UINT32							m_GeoConstantBufferSize;
	CD3DX12_GPU_DESCRIPTOR_HANDLE *	m_GeoCbvHandles;

	~ObjectD3D12Resources() 
	{
		delete[] m_GeoCbvHandles; 
	}
};

class Object
{
public:
	virtual ~Object();
	World *						m_world{ nullptr };

	Vec3						m_position;
	Vec3						m_scale;
	// TODO for rotation

	Mesh *						m_mesh{ nullptr };
	IHitable *					m_hitable{ nullptr };
	IMaterial *					m_material{ nullptr };

	ObjectD3D12Resources		m_d3dRes;

	virtual void				Update(SimpleCamera *camera, float elapsedSeconds) = 0;
	virtual void				Render(D3D12Viewer *viewer, UINT32 mid) const = 0;
	virtual AABB				BoundingBox() const = 0; // return bool as some object doesn't have bounding-box like a plane
	virtual BOOL				Hit(const Ray &r, float &t_min, float &t_max, HitRecord &out_rec) const;
	virtual void				BuildD3DRes(D3D12Viewer *viewer, CD3DX12_CPU_DESCRIPTOR_HANDLE &cbvCPUHandle, CD3DX12_GPU_DESCRIPTOR_HANDLE &cbvGPUHandle) = 0;
};

class SimpleObjectSphere : public Object
{
public:
	SimpleObjectSphere(const Vec3 &center, float radius, Mesh *mesh, IMaterial *material, World *world);

	virtual void				Update(SimpleCamera *camera, float elapsedSeconds) override;
	virtual void				Render(D3D12Viewer *viewer, UINT32 mid) const override;
	virtual AABB				BoundingBox() const override;
	virtual void				BuildD3DRes(D3D12Viewer *viewer, CD3DX12_CPU_DESCRIPTOR_HANDLE &cbvCPUHandle, CD3DX12_GPU_DESCRIPTOR_HANDLE &cbvGPUHandle) override;
};

enum SimpleObjectRectAlignAxes
{
	XY_RECT,
	XZ_RECT,
	ZY_RECT,
};

class SimpleObjectRect : public Object
{
public:
	SimpleObjectRect(SimpleObjectRectAlignAxes axes, const Vec3 &center, float width, float height, BOOL reverseFace, Mesh *mesh, IMaterial *material, World *world);

	virtual void				Update(SimpleCamera *camera, float elapsedSeconds) override;
	virtual void				Render(D3D12Viewer *viewer, UINT32 mid) const override;
	virtual AABB				BoundingBox() const override;
	virtual void				BuildD3DRes(D3D12Viewer *viewer, CD3DX12_CPU_DESCRIPTOR_HANDLE &cbvCPUHandle, CD3DX12_GPU_DESCRIPTOR_HANDLE &cbvGPUHandle) override;

	Vec3						m_min;
	Vec3						m_max;
	SimpleObjectRectAlignAxes	m_alignAxes;
	BOOL						m_reverseFace;
};

class SimpleObjectCube : public Object
{
public:
	SimpleObjectCube(const Vec3 &center, const Vec3 &size, Mesh *mesh, IMaterial *material, World *world);
	virtual ~SimpleObjectCube() override;

	virtual void				Update(SimpleCamera *camera, float elapsedSeconds) override;
	virtual void				Render(D3D12Viewer *viewer, UINT32 mid) const override;
	virtual AABB				BoundingBox() const override;
	virtual void				BuildD3DRes(D3D12Viewer *viewer, CD3DX12_CPU_DESCRIPTOR_HANDLE &cbvCPUHandle, CD3DX12_GPU_DESCRIPTOR_HANDLE &cbvGPUHandle) override;

	Vec3						m_min;
	Vec3						m_max;
	IHitable *					m_faceList[6];
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
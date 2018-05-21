#include "stdafx.h"
#include "SimpleObject.h"
#include "Hitables.h"
#include "SimpleMesh.h"
#include "D3D12Viewer.h"
#include "D3D12Helper.h"
#include "SimpleCamera.h"
#include "Materials.h"
#include "World.h"
#include "Randomizer.h"
#include "LightSources.h"
#include "std_cbuffer.h"

using namespace std;

Object::~Object()
{
	if (m_hitable)
	{
		delete m_hitable;
		m_hitable = nullptr;
	}
}


void Object::Render(D3D12Viewer *viewer, UINT32 mid) const
{
	if (m_material && m_material->GetID() == mid)
	{
		ID3D12GraphicsCommandList *commandList = viewer->GetGraphicsCommandList();

		commandList->SetGraphicsRootDescriptorTable(0, m_d3dRes.m_GeoCbvHandles[m_world->GetFrameIndex()]);
		m_material->ApplyCBV(viewer, m_world->GetLightSources()->GetIllumCbvHandle(m_world->GetFrameIndex()));
		m_material->ApplySRV(viewer);

		commandList->IASetPrimitiveTopology(ConvertPrimitiveType(m_mesh->m_primitiveType));
		commandList->IASetIndexBuffer(&m_mesh->m_d3dRes.m_indexBufferView);
		commandList->IASetVertexBuffers(0, 1, &m_mesh->m_d3dRes.m_vertexBufferView);
		commandList->DrawIndexedInstanced(m_mesh->m_indexCount, 1, 0, 0, 0);
	}
}

AABB Object::BoundingBox() const
{
	return m_hitable->BoundingBox();
}

BOOL Object::Hit(const Ray &r, float &t_min, float &t_max, HitRecord &out_rec) const
{
	BOOL hitMe = FALSE;
	if (m_hitable->Hit(r, t_min, t_max, out_rec))
	{
		t_max = out_rec.m_time;
		hitMe = TRUE;
	}

	return hitMe;
}

void Object::BuildD3DRes(D3D12Viewer *viewer, CD3DX12_CPU_DESCRIPTOR_HANDLE &cbvCPUHandle, CD3DX12_GPU_DESCRIPTOR_HANDLE &cbvGPUHandle)
{
	ID3D12Device *device = viewer->GetDevice();
	UINT32 handleOffset = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	// geo constant buffers.
	{
		m_d3dRes.m_GeoConstantBufferSize = (sizeof(GeometryConstants) + 255) & ~255;	// CB size is required to be 256-byte aligned.

																						// Create an upload heap for the geo constant buffers.
		ThrowIfFailed(device->CreateCommittedResource(
			&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
			D3D12_HEAP_FLAG_NONE,
			&CD3DX12_RESOURCE_DESC::Buffer(m_d3dRes.m_GeoConstantBufferSize * D3D12Viewer::FrameCount),
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			IID_PPV_ARGS(&m_d3dRes.m_GeoConstantBuffer)));
		m_d3dRes.m_GeoConstantBuffer->SetName(L"ObjGeoConstantBuffer");

		// Map the constant buffers. Note that unlike D3D11, the resource 
		// does not need to be unmapped for use by the GPU. In this sample, 
		// the resource stays 'permenantly' mapped to avoid overhead with 
		// mapping/unmapping each frame.
		CD3DX12_RANGE readRange(0, 0);		// We do not intend to read from this resource on the CPU.
		ThrowIfFailed(m_d3dRes.m_GeoConstantBuffer->Map(0, &readRange, reinterpret_cast<void**>(&m_d3dRes.m_pGeoConstants)));

		// Create a CBV for each frame.
		UINT64 cbOffset = 0;
		m_d3dRes.m_GeoCbvHandles = new CD3DX12_GPU_DESCRIPTOR_HANDLE[D3D12Viewer::FrameCount];
		for (UINT n = 0; n < D3D12Viewer::FrameCount; n++)
		{
			// Describe and create a constant buffer view (CBV).
			D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc = {};
			cbvDesc.BufferLocation = m_d3dRes.m_GeoConstantBuffer->GetGPUVirtualAddress() + cbOffset;
			cbvDesc.SizeInBytes = m_d3dRes.m_GeoConstantBufferSize;
			cbOffset += m_d3dRes.m_GeoConstantBufferSize;
			device->CreateConstantBufferView(&cbvDesc, cbvCPUHandle);
			m_d3dRes.m_GeoCbvHandles[n] = cbvGPUHandle;
			cbvGPUHandle.Offset(handleOffset);
			cbvCPUHandle.Offset(handleOffset);
		}
	}
}

SimpleObjectSphere::SimpleObjectSphere(const Vec3 &center, float radius, Mesh *mesh, IMaterial *material, World *world)
{
	m_translation = center;
	m_scaling = Vec3(radius, radius, radius);
	m_mesh = mesh;
	m_material = material;
	m_hitable = new TranslatedInstance(new SphereHitable(Vec3(0.0f, 0.0f, 0.0f),radius), m_translation);
	m_hitable->BindMaterial(material);
	m_world = world;
}

void SimpleObjectSphere::Update(SimpleCamera *camera, float elapsedSeconds)
{
	// update constants
	XMMATRIX scale;
	XMMATRIX trans;
	XMMATRIX mv;
	GeometryConstants geoConstants;

	trans = DirectX::XMMatrixTranslationFromVector(m_translation.m_simd);
	scale = DirectX::XMMatrixScalingFromVector(m_scaling.m_simd);
	mv = scale * trans * camera->GetViewMatrix();
	// Compute the model-view-projection matrix.
	DirectX::XMStoreFloat4x4(&geoConstants.worldViewProj, DirectX::XMMatrixTranspose(mv * camera->GetProjectionMatrix()));
	DirectX::XMStoreFloat4x4(&geoConstants.worldView, DirectX::XMMatrixTranspose(mv));

	// Copy this matrix into the appropriate location in the geo constant buffer.
	memcpy(m_d3dRes.m_pGeoConstants + m_d3dRes.m_GeoConstantBufferSize * m_world->GetFrameIndex(), &geoConstants, sizeof(GeometryConstants));
}

SimpleObjectRect::SimpleObjectRect(SimpleObjectRectAlignAxes axes, const Vec3 &center, float width, float height, BOOL reverseFace, Mesh *mesh, IMaterial *material, World *world)
	: m_alignAxes(axes)
	, m_reverseFace(reverseFace)
{
	m_translation = center;
	UINT aAxisIndex, bAxisIndex, cAxisIndex;
	switch (m_alignAxes)
	{
	case XY_RECT:
		aAxisIndex = 0;
		bAxisIndex = 1;
		cAxisIndex = 2;
		break;
	case XZ_RECT:
		aAxisIndex = 0;
		bAxisIndex = 2;
		cAxisIndex = 1;
		break;
	default:
		aAxisIndex = 2;
		bAxisIndex = 1;
		cAxisIndex = 0;
		break;
	}

	m_scaling = Vec3(width, height, 1.0f);
	//m_scale.set(aAxisIndex, width);
	//m_scale.set(bAxisIndex, height);
	//m_scale.set(cAxisIndex, 1.0f);
	Vec3 half;
	half.set(aAxisIndex, width / 2.0f);
	half.set(bAxisIndex, height / 2.0f);

	Vec3 _min = m_translation - half;
	Vec3 _max = m_translation + half;
	m_mesh = mesh;
	m_material = material;
	m_hitable = new AxisAlignedRectHitable(aAxisIndex, bAxisIndex, _min[aAxisIndex], _max[aAxisIndex], _min[bAxisIndex], _max[bAxisIndex], m_translation[cAxisIndex], m_reverseFace);
	m_hitable->BindMaterial(material);
	m_world = world;
}

void SimpleObjectRect::Update(SimpleCamera *camera, float elapsedSeconds)
{
	XMMATRIX scale;
	XMMATRIX rotate;
	XMMATRIX trans;
	XMMATRIX mv;
	GeometryConstants geoConstants;

	rotate = DirectX::XMMatrixIdentity();
	switch (m_alignAxes)
	{
	case XY_RECT:
		if (m_reverseFace)
		{
			rotate = DirectX::XMMatrixRotationY((float)M_PI);
		}
		else
		{
			rotate = DirectX::XMMatrixIdentity();
		}
		break;
	case XZ_RECT:
		if (m_reverseFace)
		{
			rotate = DirectX::XMMatrixRotationX((float)M_PI * 0.5f);
		}
		else
		{
			rotate = DirectX::XMMatrixRotationX((float)M_PI * -0.5f);
		}
		break;
	default:
		if (m_reverseFace)
		{
			rotate = DirectX::XMMatrixRotationY((float)M_PI * -0.5f);
		}
		else
		{
			rotate = DirectX::XMMatrixRotationY((float)M_PI * 0.5f);
		}
		break;
	}

	//rotate = DirectX::XMMatrixRotationX((float)M_PI / 2.0f);

	/*
	XMMATRIX    XM_CALLCONV     XMMatrixRotationX(float Angle);
	XMMATRIX    XM_CALLCONV     XMMatrixRotationY(float Angle);
	XMMATRIX    XM_CALLCONV     XMMatrixRotationZ(float Angle);
	*/

	trans = DirectX::XMMatrixTranslationFromVector(m_translation.m_simd);
	scale = DirectX::XMMatrixScalingFromVector(m_scaling.m_simd);
	mv = scale * rotate * trans * camera->GetViewMatrix();
	// Compute the model-view-projection matrix.
	DirectX::XMStoreFloat4x4(&geoConstants.worldViewProj, DirectX::XMMatrixTranspose(mv * camera->GetProjectionMatrix()));
	DirectX::XMStoreFloat4x4(&geoConstants.worldView, DirectX::XMMatrixTranspose(mv));

	// Copy this matrix into the appropriate location in the geo constant buffer.
	memcpy(m_d3dRes.m_pGeoConstants + m_d3dRes.m_GeoConstantBufferSize * m_world->GetFrameIndex(), &geoConstants, sizeof(GeometryConstants));

}

SimpleObjectCube::SimpleObjectCube(const Vec3 &center, const Vec3 &size, Mesh *mesh, IMaterial *material, World *world)
{
	m_translation = center;
	m_scaling = size;
	m_mesh = mesh;
	m_material = material;
	m_world = world;

	Vec3 _min = m_translation - m_scaling * 0.5f;
	Vec3 _max = m_translation + m_scaling * 0.5f;


	// to do, make it at center(0,0,0) and use translation
// 	m_faceList[0] = new AxisAlignedRectHitable(0, 2, -0.5f, +0.5f, -0.5f, +0.5f, 0.5f, FALSE); // up
// 	m_faceList[1] = new AxisAlignedRectHitable(0, 2, -0.5f, +0.5f, -0.5f, +0.5f, -0.5f, TRUE); // down
// 	m_faceList[2] = new AxisAlignedRectHitable(0, 1, -0.5f, +0.5f, -0.5f, +0.5f, 0.5f, FALSE); // front
// 	m_faceList[3] = new AxisAlignedRectHitable(0, 1, -0.5f, +0.5f, -0.5f, +0.5f, -0.5f, TRUE); // back
// 	m_faceList[4] = new AxisAlignedRectHitable(2, 1, -0.5f, +0.5f, -0.5f, +0.5f, 0.5f, FALSE); // right
// 	m_faceList[5] = new AxisAlignedRectHitable(2, 1, -0.5f, +0.5f, -0.5f, +0.5f, -0.5f, TRUE); // left


	m_faceList[0] = new AxisAlignedRectHitable(0, 2, _min.x(), _max.x(), _min.z(), _max.z(), _max.y(), FALSE); // up
	m_faceList[1] = new AxisAlignedRectHitable(0, 2, _min.x(), _max.x(), _min.z(), _max.z(), _min.y(), TRUE); // down
	m_faceList[2] = new AxisAlignedRectHitable(0, 1, _min.x(), _max.x(), _min.y(), _max.y(), _max.z(), FALSE); // front
	m_faceList[3] = new AxisAlignedRectHitable(0, 1, _min.x(), _max.x(), _min.y(), _max.y(), _min.z(), TRUE); // back
	m_faceList[4] = new AxisAlignedRectHitable(2, 1, _min.z(), _max.z(), _min.y(), _max.y(), _max.x(), FALSE); // right
	m_faceList[5] = new AxisAlignedRectHitable(2, 1, _min.z(), _max.z(), _min.y(), _max.y(), _min.x(), TRUE); // left

	m_hitable = new HitableCombo(&m_faceList[0], 6);
	m_hitable->BindMaterial(material);
}

SimpleObjectCube::~SimpleObjectCube()
{
	for (UINT32 i = 0; i < 6; ++i)
	{
		if (m_faceList[i])
		{
			delete m_faceList[i];
			m_faceList[i] = nullptr;
		}
	}
}

void SimpleObjectCube::Update(SimpleCamera *camera, float elapsedSeconds)
{
	// update constants
	XMMATRIX scale;
	XMMATRIX trans;
	XMMATRIX mv;
	GeometryConstants geoConstants;

	trans = DirectX::XMMatrixTranslationFromVector(m_translation.m_simd);
	scale = DirectX::XMMatrixScalingFromVector(m_scaling.m_simd);
	mv = scale * trans * camera->GetViewMatrix();
	// Compute the model-view-projection matrix.
	DirectX::XMStoreFloat4x4(&geoConstants.worldViewProj, DirectX::XMMatrixTranspose(mv * camera->GetProjectionMatrix()));
	DirectX::XMStoreFloat4x4(&geoConstants.worldView, DirectX::XMMatrixTranspose(mv));

	// Copy this matrix into the appropriate location in the geo constant buffer.
	memcpy(m_d3dRes.m_pGeoConstants + m_d3dRes.m_GeoConstantBufferSize * m_world->GetFrameIndex(), &geoConstants, sizeof(GeometryConstants));
}

SimpleObjectBVHNode::SimpleObjectBVHNode(std::vector<Object *> objects)
{
	assert(!objects.empty());

	// sort objects with order
	UINT32 axis = UINT32(3 * Randomizer::RandomUNorm());
	if (axis == 0)
		sort(objects.begin(), objects.end(), [](Object * a, Object * b) { return b->m_translation.x() < a->m_translation.x(); });
	else if(axis == 1)
		sort(objects.begin(), objects.end(), [](Object * a, Object * b) { return b->m_translation.y() < a->m_translation.y(); });
	else
		sort(objects.begin(), objects.end(), [](Object * a, Object * b) { return b->m_translation.z() < a->m_translation.z(); });

	// divide into two
	vector<Object *>::size_type fullsize = objects.size();
	if (fullsize == 1)
	{
		leftChild = objects[0];
		rightChild = nullptr;

		m_bindingBox = leftChild->BoundingBox();
	}
	else if (fullsize == 2)
	{
		leftChild = objects[0];
		rightChild = objects[1];

		// generate binding box by combine 2 children's
		m_bindingBox = CombineAABB(leftChild->BoundingBox(), rightChild->BoundingBox());
	}
	else if (fullsize > 2)
	{
		vector<Object *>::size_type halfsize = static_cast<vector<Object *>::size_type>(fullsize * 0.5f);
		vector<Object *> leftObjects;
		vector<Object *> rightObjects;

		for (auto i = 0; i < fullsize; ++i)
		{
			if (i < halfsize)
				leftObjects.push_back(objects[i]);
			else
				rightObjects.push_back(objects[i]);
		}
		leftChild = new SimpleObjectBVHNode(leftObjects);
		rightChild = new SimpleObjectBVHNode(rightObjects);

		// generate binding box by combine 2 children's
		m_bindingBox = CombineAABB(leftChild->BoundingBox(), rightChild->BoundingBox());
	}
	else
	{
		assert(FALSE);
	}
}

SimpleObjectBVHNode::~SimpleObjectBVHNode()
{
	if (leftChild)
	{
		delete leftChild;
		leftChild = nullptr;
	}

	if (rightChild)
	{
		delete rightChild;
		rightChild = nullptr;
	}
}

void SimpleObjectBVHNode::Update(SimpleCamera *camera, float elapsedSeconds)
{
	if (leftChild)
	{
		leftChild->Update(camera, elapsedSeconds);
	}

	if (rightChild)
	{
		rightChild->Update(camera, elapsedSeconds);
	}
}

void SimpleObjectBVHNode::Render(D3D12Viewer *viewer, UINT32 mid) const
{
	if (leftChild)
	{
		leftChild->Render(viewer, mid);
	}

	if (rightChild)
	{
		rightChild->Render(viewer, mid);
	}
}

BOOL SimpleObjectBVHNode::Hit(const Ray &r, float &t_min, float &t_max, HitRecord &out_rec) const
{
	BOOL hitMe = FALSE;
	if (m_bindingBox.Hit(r, t_min, t_max))
	{
		if (leftChild) 
		{
			hitMe |= leftChild->Hit(r, t_min, t_max, out_rec);
		}

		if (rightChild)
		{
			hitMe |= rightChild->Hit(r, t_min, t_max, out_rec);
		}
	}

	return hitMe;
}

AABB SimpleObjectBVHNode::BoundingBox() const
{
	return m_bindingBox;
}

void SimpleObjectBVHNode::BuildD3DRes(D3D12Viewer *viewer, CD3DX12_CPU_DESCRIPTOR_HANDLE &cbvCPUHandle, CD3DX12_GPU_DESCRIPTOR_HANDLE &cbvGPUHandle)
{
	if (leftChild)
	{
		leftChild->BuildD3DRes(viewer, cbvCPUHandle, cbvGPUHandle);
	}

	if (rightChild)
	{
		rightChild->BuildD3DRes(viewer, cbvCPUHandle, cbvGPUHandle);
	}
}


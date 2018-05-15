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

using namespace std;

SimpleObjectSphere::SimpleObjectSphere(const Vec3 &center, float radius, Mesh *mesh, IMaterial *material, World *world)
{
	m_position = center;
	m_scale = radius;
	m_mesh = mesh;
	m_material = material;
	SphereHitable *hitable = new SphereHitable(center, radius);
	hitable->BindMaterial(material);
	m_hitable = hitable;
	m_world = world;
}

SimpleObjectSphere::~SimpleObjectSphere()
{
	if (m_hitable)
	{
		delete m_hitable;
		m_hitable = nullptr;
	}
}

void SimpleObjectSphere::Update(SimpleCamera *camera, float elapsedSeconds)
{
	// update constants
	XMMATRIX scale;
	XMMATRIX trans;
	XMMATRIX mv;
	GeometryConstants geoConstants;

	trans = DirectX::XMMatrixTranslationFromVector(m_position.m_simd);
	scale = DirectX::XMMatrixScaling(m_scale, m_scale, m_scale);
	mv = scale * trans * camera->GetViewMatrix();
	// Compute the model-view-projection matrix.
	DirectX::XMStoreFloat4x4(&geoConstants.mWorldViewProj, DirectX::XMMatrixTranspose(mv * camera->GetProjectionMatrix()));
	DirectX::XMStoreFloat4x4(&geoConstants.mWorldView, DirectX::XMMatrixTranspose(mv));

	// Copy this matrix into the appropriate location in the geo constant buffer.
	memcpy(m_d3dRes.m_pGeoConstants + m_d3dRes.m_GeoConstantBufferSize * m_world->GetFrameIndex(), &geoConstants, sizeof(GeometryConstants));

	// copy material data into the appropriate location in the mtl constant buffer
	memcpy(m_d3dRes.m_pMtlConstants + m_d3dRes.m_MtlConstantBufferSize * m_world->GetFrameIndex(), m_material->GetDataPtr(), m_material->GetDataSize());
}

void SimpleObjectSphere::Render(D3D12Viewer *viewer, UINT32 mid) const
{
	if (m_material && m_material->GetID() == mid)
	{
		ID3D12GraphicsCommandList *commandList = viewer->GetGraphicsCommandList();

		commandList->SetGraphicsRootDescriptorTable(0, m_d3dRes.m_GeoCbvHandles[m_world->GetFrameIndex()]);
		commandList->SetGraphicsRootDescriptorTable(1, m_d3dRes.m_MtlCbvHandles[m_world->GetFrameIndex()]);
		commandList->SetGraphicsRootDescriptorTable(2, m_world->GetIllumCbvHandle(m_world->GetFrameIndex()));

		commandList->IASetPrimitiveTopology(ConvertPrimitiveType(m_mesh->m_primitiveType));
		commandList->IASetIndexBuffer(&m_mesh->m_d3dRes.m_indexBufferView);
		commandList->IASetVertexBuffers(0, 1, &m_mesh->m_d3dRes.m_vertexBufferView);
		commandList->DrawIndexedInstanced(m_mesh->m_indexCount, 1, 0, 0, 0);
	}
}

AABB SimpleObjectSphere::BoundingBox() const
{
	return AABB(m_position - Vec3(m_scale, m_scale, m_scale), m_position + Vec3(m_scale, m_scale, m_scale));
}

BOOL SimpleObjectSphere::Hit(const Ray &r, float &t_min, float &t_max, HitRecord &rec) const
{
	BOOL hitMe = FALSE;
	if (m_hitable->Hit(r, t_min, t_max, rec))
	{
		t_max = rec.m_time;
		hitMe = TRUE;
	}

	return hitMe;
}

void SimpleObjectSphere::BuildD3DRes(D3D12Viewer *viewer, CD3DX12_CPU_DESCRIPTOR_HANDLE &cbvCPUHandle, CD3DX12_GPU_DESCRIPTOR_HANDLE &cbvGPUHandle)
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

	// mtl constant buffers.
	{
		m_d3dRes.m_MtlConstantBufferSize = (m_material->GetDataSize() + 255) & ~255;	// CB size is required to be 256-byte aligned.

		// Create an upload heap for the mtl constant buffers.
		ThrowIfFailed(device->CreateCommittedResource(
			&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
			D3D12_HEAP_FLAG_NONE,
			&CD3DX12_RESOURCE_DESC::Buffer(m_d3dRes.m_MtlConstantBufferSize * D3D12Viewer::FrameCount),
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			IID_PPV_ARGS(&m_d3dRes.m_MtlConstantBuffer)));
		m_d3dRes.m_MtlConstantBuffer->SetName(L"ObjMtlConstantBuffer");

		CD3DX12_RANGE readRange(0, 0);		// We do not intend to read from this resource on the CPU.
		ThrowIfFailed(m_d3dRes.m_MtlConstantBuffer->Map(0, &readRange, reinterpret_cast<void**>(&m_d3dRes.m_pMtlConstants)));

		// Create a CBV for each frame.
		UINT64 cbOffset = 0;
		m_d3dRes.m_MtlCbvHandles = new CD3DX12_GPU_DESCRIPTOR_HANDLE[D3D12Viewer::FrameCount];
		for (UINT n = 0; n < D3D12Viewer::FrameCount; n++)
		{
			// Describe and create a constant buffer view (CBV).
			D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc = {};
			cbvDesc.BufferLocation = m_d3dRes.m_MtlConstantBuffer->GetGPUVirtualAddress() + cbOffset;
			cbvDesc.SizeInBytes = m_d3dRes.m_MtlConstantBufferSize;
			cbOffset += m_d3dRes.m_MtlConstantBufferSize;
			device->CreateConstantBufferView(&cbvDesc, cbvCPUHandle);
			m_d3dRes.m_MtlCbvHandles[n] = cbvGPUHandle;
			cbvGPUHandle.Offset(handleOffset);
			cbvCPUHandle.Offset(handleOffset);
		}
	}	
}


SimpleObjectBVHNode::SimpleObjectBVHNode(std::vector<Object *> objects)
{
	assert(!objects.empty());

	// sort objects with order
	UINT32 axis = UINT32(3 * Randomizer::RandomUNorm());
	if (axis == 0)
		sort(objects.begin(), objects.end(), [](Object * a, Object * b) { return b->m_position.x() < a->m_position.x(); });
	else if(axis == 1)
		sort(objects.begin(), objects.end(), [](Object * a, Object * b) { return b->m_position.y() < a->m_position.y(); });
	else
		sort(objects.begin(), objects.end(), [](Object * a, Object * b) { return b->m_position.z() < a->m_position.z(); });

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

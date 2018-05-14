#include "stdafx.h"
#include "SimpleObject.h"
#include "Hitables.h"
#include "SimpleMesh.h"
#include "D3D12Viewer.h"
#include "D3D12Helper.h"
#include "SimpleCamera.h"
#include "Materials.h"
#include "World.h"

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

void SimpleObjectSphere::Render(D3D12Viewer *viewer) const
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

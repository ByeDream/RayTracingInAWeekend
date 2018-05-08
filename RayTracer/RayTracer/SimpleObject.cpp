#include "stdafx.h"
#include "SimpleObject.h"
#include "Hitables.h"
#include "SimpleMesh.h"
#include "D3D12Viewer.h"
#include "D3D12Helper.h"
#include "SimpleCamera.h"

SimpleSphereObject::SimpleSphereObject(const Vec3 &center, float radius, Mesh *mesh, IMaterial *material)
{
	m_position = center;
	m_scale = radius;
	m_mesh = mesh;
	m_material = material;
	SphereHitable *hitable = new SphereHitable(center, radius);
	hitable->BindMaterial(material);
	m_hitable = hitable;
}

SimpleSphereObject::~SimpleSphereObject()
{
	if (m_hitable)
	{
		delete m_hitable;
		m_hitable = nullptr;
	}
}

void SimpleSphereObject::Update(SimpleCamera *camera, float elapsedSeconds)
{
	m_d3dRes.m_VSCurrentCbvIndex = (m_d3dRes.m_VSCurrentCbvIndex + 1) % D3D12Viewer::FrameCount;

	XMMATRIX model;
	XMFLOAT4X4 mvp;

	model = DirectX::XMMatrixTranslationFromVector(m_position.m_simd);
	// Compute the model-view-projection matrix.
	DirectX::XMStoreFloat4x4(&mvp, DirectX::XMMatrixTranspose(model * camera->GetViewMatrix() * camera->GetProjectionMatrix()));

	// Copy this matrix into the appropriate location in the upload heap subresource.
	memcpy(&m_d3dRes.m_pVSConstants[m_d3dRes.m_VSCurrentCbvIndex], &mvp, sizeof(mvp));
}

void SimpleSphereObject::Render(D3D12Viewer *viewer) const
{
	ID3D12GraphicsCommandList *commandList = viewer->GetGraphicsCommandList();

	ID3D12DescriptorHeap* ppHeaps[] = { m_d3dRes.m_VSCbvHeap.Get() };
	commandList->SetDescriptorHeaps(_countof(ppHeaps), ppHeaps);
	UINT32 handleOffset = viewer->GetDevice()->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	CD3DX12_GPU_DESCRIPTOR_HANDLE cbvHandle(m_d3dRes.m_VSCbvHeap->GetGPUDescriptorHandleForHeapStart(), m_d3dRes.m_VSCurrentCbvIndex, handleOffset);
	commandList->SetGraphicsRootDescriptorTable(0, cbvHandle);

	commandList->IASetPrimitiveTopology(ConvertPrimitiveType(m_mesh->m_primitiveType));
	commandList->IASetIndexBuffer(&m_mesh->m_d3dRes.m_indexBufferView);
	commandList->IASetVertexBuffers(0, 1, &m_mesh->m_d3dRes.m_vertexBufferView);
	commandList->DrawIndexedInstanced(m_mesh->m_indexCount, 1, 0, 0, 0);
}

void SimpleSphereObject::BuildD3DRes(D3D12Viewer *viewer)
{
	ID3D12Device *device = viewer->GetDevice();
	{
		D3D12_DESCRIPTOR_HEAP_DESC cbvHeapDesc = {};
		cbvHeapDesc.NumDescriptors = D3D12Viewer::FrameCount;
		cbvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
		cbvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
		ThrowIfFailed(device->CreateDescriptorHeap(&cbvHeapDesc, IID_PPV_ARGS(&m_d3dRes.m_VSCbvHeap)));
	}

	{
		// Create an upload heap for the constant buffers.
		ThrowIfFailed(device->CreateCommittedResource(
			&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
			D3D12_HEAP_FLAG_NONE,
			&CD3DX12_RESOURCE_DESC::Buffer(sizeof(VSConstants) * D3D12Viewer::FrameCount),
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			IID_PPV_ARGS(&m_d3dRes.m_VSConstantBuffer)));

		// Map the constant buffers. Note that unlike D3D11, the resource 
		// does not need to be unmapped for use by the GPU. In this sample, 
		// the resource stays 'permenantly' mapped to avoid overhead with 
		// mapping/unmapping each frame.
		CD3DX12_RANGE readRange(0, 0);		// We do not intend to read from this resource on the CPU.
		ThrowIfFailed(m_d3dRes.m_VSConstantBuffer->Map(0, &readRange, reinterpret_cast<void**>(&m_d3dRes.m_pVSConstants)));
	}

	{
		CD3DX12_CPU_DESCRIPTOR_HANDLE cbvHandle(m_d3dRes.m_VSCbvHeap->GetCPUDescriptorHandleForHeapStart());
		// Create a RTV for each frame.
		UINT64 cbOffset = 0;
		UINT32 handleOffset = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		for (UINT n = 0; n < D3D12Viewer::FrameCount; n++)
		{
			// Describe and create a constant buffer view (CBV).
			D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc = {};
			cbvDesc.BufferLocation = m_d3dRes.m_VSConstantBuffer->GetGPUVirtualAddress() + cbOffset;
			cbvDesc.SizeInBytes = sizeof(VSConstants);
			cbOffset += cbvDesc.SizeInBytes;
			device->CreateConstantBufferView(&cbvDesc, cbvHandle);
			cbvHandle.Offset(handleOffset);
		}

		m_d3dRes.m_VSCurrentCbvIndex = 0;
	}
}

#include "stdafx.h"
#include "SimpleMesh.h"
#include "D3D12Viewer.h"
#include "D3D12Helper.h"

D3D12_INPUT_ELEMENT_DESC SimpleMesh::D3DVertexDeclaration[] =
{
	{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0,  D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
	{ "NORMAL",   0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
	{ "TANGENT",  0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 24, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
	{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 40, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
};

UINT32 SimpleMesh::D3DVertexDeclarationElementCount = _countof(D3DVertexDeclaration);

void Mesh::BuildD3DRes(D3D12Viewer *viewer)
{
	ID3D12Device *device = viewer->GetDevice();
	ID3D12GraphicsCommandList *commandList = viewer->GetGraphicsCommandList();

	viewer->ResetCommandList();

	// make sure the GPU finish the upload before leave the scope.
	ComPtr<ID3D12Resource> vertexBufferUploadHeap;
	ComPtr<ID3D12Resource> indexBufferUploadHeap;

	// Create the vertex buffer.
	{
		ThrowIfFailed(device->CreateCommittedResource(
			&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
			D3D12_HEAP_FLAG_NONE,
			&CD3DX12_RESOURCE_DESC::Buffer(m_vertexBufferSize),
			D3D12_RESOURCE_STATE_COPY_DEST,
			nullptr,
			IID_PPV_ARGS(&m_d3dRes.m_vertexBufferHeap)));

		NAME_D3D12_OBJECT(m_d3dRes.m_vertexBufferHeap);

		ThrowIfFailed(device->CreateCommittedResource(
			&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
			D3D12_HEAP_FLAG_NONE,
			&CD3DX12_RESOURCE_DESC::Buffer(m_vertexBufferSize),
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			IID_PPV_ARGS(&vertexBufferUploadHeap)));

		// Copy data to the intermediate upload heap and then schedule a copy 
		// from the upload heap to the vertex buffer.
		D3D12_SUBRESOURCE_DATA vertexData = {};
		vertexData.pData = m_vertexBuffer;
		vertexData.RowPitch = m_vertexBufferSize;
		vertexData.SlicePitch = vertexData.RowPitch;

		UpdateSubresources<1>(commandList, m_d3dRes.m_vertexBufferHeap.Get(), vertexBufferUploadHeap.Get(), 0, 0, 1, &vertexData);
		commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(m_d3dRes.m_vertexBufferHeap.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER));

		// Initialize the vertex buffer view.
		m_d3dRes.m_vertexBufferView.BufferLocation = m_d3dRes.m_vertexBufferHeap->GetGPUVirtualAddress();
		m_d3dRes.m_vertexBufferView.StrideInBytes = m_vertexStride;
		m_d3dRes.m_vertexBufferView.SizeInBytes = m_vertexBufferSize;
	}


	// Create the index buffer.
	{
		ThrowIfFailed(device->CreateCommittedResource(
			&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
			D3D12_HEAP_FLAG_NONE,
			&CD3DX12_RESOURCE_DESC::Buffer(m_indexBufferSize),
			D3D12_RESOURCE_STATE_COPY_DEST,
			nullptr,
			IID_PPV_ARGS(&m_d3dRes.m_indexBufferHeap)));

		NAME_D3D12_OBJECT(m_d3dRes.m_indexBufferHeap);

		ThrowIfFailed(device->CreateCommittedResource(
			&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
			D3D12_HEAP_FLAG_NONE,
			&CD3DX12_RESOURCE_DESC::Buffer(m_indexBufferSize),
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			IID_PPV_ARGS(&indexBufferUploadHeap)));

		// Copy data to the intermediate upload heap and then schedule a copy 
		// from the upload heap to the index buffer.
		D3D12_SUBRESOURCE_DATA indexData = {};
		indexData.pData = m_indexBuffer;
		indexData.RowPitch = m_indexBufferSize;
		indexData.SlicePitch = indexData.RowPitch;

		UpdateSubresources<1>(commandList, m_d3dRes.m_indexBufferHeap.Get(), indexBufferUploadHeap.Get(), 0, 0, 1, &indexData);
		commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(m_d3dRes.m_indexBufferHeap.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_INDEX_BUFFER));

		// Describe the index buffer view.
		m_d3dRes.m_indexBufferView.BufferLocation = m_d3dRes.m_indexBufferHeap->GetGPUVirtualAddress();
		m_d3dRes.m_indexBufferView.Format = (m_indexType == kIndexSize16) ? DXGI_FORMAT_R16_UINT : DXGI_FORMAT_R32_UINT;
		m_d3dRes.m_indexBufferView.SizeInBytes = m_indexBufferSize;
	}

	viewer->ExecuteCommandList();
	viewer->WaitForGpu();
}


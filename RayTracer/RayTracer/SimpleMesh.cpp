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

	// Note: ComPtr's are CPU objects but this resource needs to stay in scope until
	// the command list that references it has finished executing on the GPU.
	// We will flush the GPU at the end of this method to ensure the resource is not
	// prematurely destroyed.
	ComPtr<ID3D12Resource> vertexBufferUploadHeap;
	ComPtr<ID3D12Resource> indexBufferUploadHeap;

	// Create the vertex buffer.
	{
		viewer->CreateAndUnloadBuffer(m_d3dRes.m_vertexBufferHeap, vertexBufferUploadHeap, m_vertexBuffer, m_vertexBufferSize);

		// Initialize the vertex buffer view.
		m_d3dRes.m_vertexBufferView.BufferLocation = m_d3dRes.m_vertexBufferHeap->GetGPUVirtualAddress();
		m_d3dRes.m_vertexBufferView.StrideInBytes = m_vertexStride;
		m_d3dRes.m_vertexBufferView.SizeInBytes = m_vertexBufferSize;
	}


	// Create the index buffer.
	{
		viewer->CreateAndUnloadBuffer(m_d3dRes.m_indexBufferHeap, indexBufferUploadHeap, m_indexBuffer, m_indexBufferSize);
		
		// Describe the index buffer view.
		m_d3dRes.m_indexBufferView.BufferLocation = m_d3dRes.m_indexBufferHeap->GetGPUVirtualAddress();
		m_d3dRes.m_indexBufferView.Format = (m_indexType == kIndexSize16) ? DXGI_FORMAT_R16_UINT : DXGI_FORMAT_R32_UINT;
		m_d3dRes.m_indexBufferView.SizeInBytes = m_indexBufferSize;
	}

	viewer->ExecuteCommandList();
	viewer->WaitForGpu();
}


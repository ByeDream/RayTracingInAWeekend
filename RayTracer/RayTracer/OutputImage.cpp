#include "stdafx.h"
#include "OutputImage.h"
#include "PPMImageMaker.h"
#include "Vec3.h"
#include "D3D12Viewer.h"
#include "D3D12Helper.h"

OutputImage::OutputImage(UINT32 width, UINT32 height, const char *name)
	: m_width(width)
	, m_height(height)
	, m_name(name)
{
	m_aspectRatio = static_cast<float>(width) / static_cast<float>(height);

	m_dataSizeInByte = (UINT64)m_width * (UINT64)m_height * (UINT64)m_pixelSizeInByte;
	if (m_dataSizeInByte)
		m_data = new UINT8[m_dataSizeInByte];
	memset(m_data, 0, m_dataSizeInByte);
}


OutputImage::~OutputImage()
{
	if (m_resolvePipelineState)
	{
		delete m_resolvePipelineState;
		m_resolvePipelineState = nullptr;
	}

	if (m_data)
	{
		delete[] m_data;
		m_data = nullptr;
	}
}

void OutputImage::RenderAsRainbow()
{
	for (UINT32 j = 0; j < m_height; j++)
	{
		for (UINT32 i = 0; i < m_width; i++)
		{
			Vec3 col(float(i) / float(m_width - 1), 1.0f - float(j) / float(m_height - 1), 0.2f);
			float a = 1.0f;

			UINT8 *baseOffset = m_data + (j * m_width + i) * m_pixelSizeInByte;
			*baseOffset = static_cast<UINT32>(col.r() * 255.0f) & 0xFF;
			*(baseOffset + 1) = static_cast<UINT32>(col.g() * 255.0f) & 0xFF;
			*(baseOffset + 2) = static_cast<UINT32>(col.b() * 255.0f) & 0xFF;
			*(baseOffset + 3) = static_cast<UINT32>(a * 255.0f) & 0xFF;
		}
	}

	m_isDirty = TRUE;
}

void OutputImage::RenderAsRed()
{
	for (UINT32 i = 0; i < m_width * m_height; i++)
	{
		UINT32 *baseOffset = reinterpret_cast<UINT32 *>(m_data + i * m_pixelSizeInByte);
		*(baseOffset) = 0xFF0000FF;
	}

	m_isDirty = TRUE;
}

void OutputImage::Render(const Vec3 *pixels, UINT32 pixelCount)
{
	if (pixelCount)
	{
		bool checkSize = pixelCount == (m_width * m_height);
		assert(checkSize && "unmatching image size and data size");
	}

	for (UINT32 j = 0; j < m_height; j++)
	{
		for (UINT32 i = 0; i < m_width; i++)
		{
			UINT32 index = j * m_width + i;
			const Vec3 &col = pixels[index];
			float a = 1.0f;

			UINT8 *baseOffset = m_data + index * m_pixelSizeInByte;
			*baseOffset = static_cast<UINT32>(col.r() * 255.0f) & 0xFF;
			*(baseOffset + 1) = static_cast<UINT32>(col.g() * 255.0f) & 0xFF;
			*(baseOffset + 2) = static_cast<UINT32>(col.b() * 255.0f) & 0xFF;
			*(baseOffset + 3) = static_cast<UINT32>(a * 255.0f) & 0xFF;
		}
	}

	m_isDirty = TRUE;
}

void OutputImage::Upload(D3D12Viewer *viewer)
{
	ID3D12GraphicsCommandList *commandList = viewer->GetGraphicsCommandList();
	// Copy data to the intermediate upload heap and then schedule a copy
	// from the upload heap to the Texture2D.
	commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(m_resolveTargetTexture.Get(), D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_COPY_DEST));

	D3D12_SUBRESOURCE_DATA textureData = {};
	textureData.pData = m_data;
	textureData.RowPitch = m_width * m_pixelSizeInByte;
	textureData.SlicePitch = textureData.RowPitch * m_height;

	UpdateSubresources(commandList, m_resolveTargetTexture.Get(), m_resolveTargetTextureUploadHeap.Get(), 0, 0, 1, &textureData);
	commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(m_resolveTargetTexture.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE));
}

void OutputImage::Resolve(D3D12Viewer *viewer)
{
	ID3D12GraphicsCommandList *commandList = viewer->GetGraphicsCommandList();
	// Set necessary state.
	commandList->SetPipelineState(m_resolvePipelineState->m_PSO.Get());
	commandList->SetGraphicsRootSignature(m_resolvePipelineState->m_RS.Get());
	ID3D12DescriptorHeap* ppHeaps[] = { m_resolveTargetTextureSRVHeap.Get() };
	commandList->SetDescriptorHeaps(_countof(ppHeaps), ppHeaps);

	commandList->SetGraphicsRootDescriptorTable(0, m_resolveTargetTextureSRVHeap->GetGPUDescriptorHandleForHeapStart());

	// Record commands.
	commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	commandList->IASetVertexBuffers(0, 1, &m_vertexBufferView);
	commandList->DrawInstanced(3, 1, 0, 0);
}

void OutputImage::BuildD3DRes(D3D12Viewer *viewer)
{
	viewer->ResetCommandList();
	
	// Create pipeline state obj of resolving draw
	{
		CD3DX12_DESCRIPTOR_RANGE1 ranges[1];
		ranges[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0, 0, D3D12_DESCRIPTOR_RANGE_FLAG_DATA_STATIC_WHILE_SET_AT_EXECUTE); // not D3D12_DESCRIPTOR_RANGE_FLAG_DATA_STATIC, this texture will be updated sometime

		CD3DX12_ROOT_PARAMETER1 rootParameters[1];
		rootParameters[0].InitAsDescriptorTable(1, &ranges[0], D3D12_SHADER_VISIBILITY_PIXEL);

		D3D12_STATIC_SAMPLER_DESC sampler = {};
		sampler.Filter = D3D12_FILTER_MIN_MAG_MIP_POINT;
		sampler.AddressU = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
		sampler.AddressV = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
		sampler.AddressW = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
		sampler.MipLODBias = 0;
		sampler.MaxAnisotropy = 0;
		sampler.ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
		sampler.BorderColor = D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK;
		sampler.MinLOD = 0.0f;
		sampler.MaxLOD = D3D12_FLOAT32_MAX;
		sampler.ShaderRegister = 0;
		sampler.RegisterSpace = 0;
		sampler.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

		CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC rootSignatureDesc;
		rootSignatureDesc.Init_1_1(_countof(rootParameters), rootParameters, 1, &sampler, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

		// Define the vertex input layout.
		D3D12_INPUT_ELEMENT_DESC inputElementDescs[] =
		{
			{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
			{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
		};
		D3D12_INPUT_LAYOUT_DESC inputLayout{ inputElementDescs, _countof(inputElementDescs) };

		m_resolvePipelineState = viewer->CreatePipelineState(rootSignatureDesc, L"..\\Assets\\resloveImage.hlsl", L"..\\Assets\\resloveImage.hlsl", inputLayout, FALSE, FALSE, FALSE);
	}

	// Create the vertex buffer of resolving draw.
	// Note: ComPtr's are CPU objects but this resource needs to stay in scope until
	// the command list that references it has finished executing on the GPU.
	// We will flush the GPU at the end of this method to ensure the resource is not
	// prematurely destroyed.
	ComPtr<ID3D12Resource> vertexUploadHeap;
	{
		// Define the geometry for a triangle.
		//(-1, 1) -------------- ( 1, 1)
		//   |                      |
		//   |                      |
		//   |                      |
		//(-1,-1) -------------- ( 1,-1)
		struct Vertex
		{
			XMFLOAT3 position;
			XMFLOAT2 uv;
		};

		Vertex triangleVertices[] =
		{
			{ { -1.0f, 3.0f, 0.0f },{ 0.0f, -1.0f } },
			{ { 3.0f, -1.0f, 0.0f },{ 2.0f, 1.0f } },
			{ { -1.0f, -1.0f, 0.0f },{ 0.0f, 1.0f } }
		};
		const UINT32 vertexBufferSize = sizeof(triangleVertices);

		viewer->CreateAndUnloadBuffer(m_vertexBuffer, vertexUploadHeap, triangleVertices, vertexBufferSize);

		// Initialize the vertex buffer view.
		m_vertexBufferView.BufferLocation = m_vertexBuffer->GetGPUVirtualAddress();
		m_vertexBufferView.StrideInBytes = sizeof(Vertex);
		m_vertexBufferView.SizeInBytes = vertexBufferSize;
	}
	// Create the SRV of resolving draw.
	{
		ID3D12Device *device = viewer->GetDevice();
		// Describe and create a Texture2D.
		D3D12_RESOURCE_DESC textureDesc = {};
		textureDesc.MipLevels = 1;
		textureDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		textureDesc.Width = m_width;	// full screen size
		textureDesc.Height = m_height;
		textureDesc.Flags = D3D12_RESOURCE_FLAG_NONE;
		textureDesc.DepthOrArraySize = 1;
		textureDesc.SampleDesc.Count = 1;
		textureDesc.SampleDesc.Quality = 0;
		textureDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;

		ThrowIfFailed(device->CreateCommittedResource(
			&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
			D3D12_HEAP_FLAG_NONE,
			&textureDesc,
			D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
			nullptr,
			IID_PPV_ARGS(&m_resolveTargetTexture)));
		NAME_D3D12_OBJECT(m_resolveTargetTexture);


		// Create the GPU upload buffer.
		UINT64 uploadBufferSize = GetRequiredIntermediateSize(m_resolveTargetTexture.Get(), 0, 1);
		ThrowIfFailed(device->CreateCommittedResource(
			&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
			D3D12_HEAP_FLAG_NONE,
			&CD3DX12_RESOURCE_DESC::Buffer(uploadBufferSize),
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			IID_PPV_ARGS(&m_resolveTargetTextureUploadHeap)));
		NAME_D3D12_OBJECT(m_resolveTargetTextureUploadHeap);

		// Describe and create a SRV for the texture.
		// Describe and create a shader resource view (SRV) heap for the texture.
		D3D12_DESCRIPTOR_HEAP_DESC srvHeapDesc = {};
		srvHeapDesc.NumDescriptors = 1;
		srvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
		srvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
		ThrowIfFailed(device->CreateDescriptorHeap(&srvHeapDesc, IID_PPV_ARGS(&m_resolveTargetTextureSRVHeap)));
		NAME_D3D12_OBJECT(m_resolveTargetTextureSRVHeap);

		D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
		srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		srvDesc.Format = textureDesc.Format;
		srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
		srvDesc.Texture2D.MipLevels = 1;
		viewer->GetDevice()->CreateShaderResourceView(m_resolveTargetTexture.Get(), &srvDesc, m_resolveTargetTextureSRVHeap->GetCPUDescriptorHandleForHeapStart());
	}
	// Close the command list and execute it to begin the initial GPU setup.
	viewer->ExecuteCommandList();
	viewer->WaitForGpu();
}

void OutputImage::Output()
{
	std::string ppmFileName = m_name + ".ppm";
	PPMImageMaker::OutputRGBA8ToFile(ppmFileName.c_str(), m_width, m_height, m_data, m_dataSizeInByte);
}
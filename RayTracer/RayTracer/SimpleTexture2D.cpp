#include "stdafx.h"
#include "SimpleTexture2D.h"

#include "D3D12Viewer.h"
#include "D3D12Helper.h"

#include "FileIO.h"
#include "tga_reader.h"

void SimpleTexture2D::BuildD3DRes(D3D12Viewer *viewer, CD3DX12_CPU_DESCRIPTOR_HANDLE &srvCPUHandle, CD3DX12_GPU_DESCRIPTOR_HANDLE &srvGPUHandle)
{
	ID3D12Device *device = viewer->GetDevice();
	UINT32 handleOffset = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	ID3D12GraphicsCommandList *commandList = viewer->GetGraphicsCommandList();

	viewer->ResetCommandList();

	// Describe and create a Texture2D.
	D3D12_RESOURCE_DESC textureDesc = {};
	textureDesc.MipLevels = 1;
	textureDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	textureDesc.Width = m_width;
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
		IID_PPV_ARGS(&m_d3dRes.m_texture)));
	m_d3dRes.m_texture->SetName(L"simpleTexture2D");

	ComPtr<ID3D12Resource>	simpleTexture2DUploadHeap;
	// Create the GPU upload buffer.
	UINT64 uploadBufferSize = GetRequiredIntermediateSize(m_d3dRes.m_texture.Get(), 0, 1);
	ThrowIfFailed(device->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(uploadBufferSize),
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&simpleTexture2DUploadHeap)));
	NAME_D3D12_OBJECT(simpleTexture2DUploadHeap);

	// Copy data to the intermediate upload heap and then schedule a copy
	// from the upload heap to the Texture2D.
	commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(m_d3dRes.m_texture.Get(), D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_COPY_DEST));

	D3D12_SUBRESOURCE_DATA textureData = {};
	textureData.pData = m_pixelData;
	textureData.RowPitch = m_width * 4; //RGBA
	textureData.SlicePitch = textureData.RowPitch * m_height;

	UpdateSubresources(commandList, m_d3dRes.m_texture.Get(), simpleTexture2DUploadHeap.Get(), 0, 0, 1, &textureData);
	commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(m_d3dRes.m_texture.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE));

	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.Format = textureDesc.Format;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MipLevels = 1;
	device->CreateShaderResourceView(m_d3dRes.m_texture.Get(), &srvDesc, srvCPUHandle);
	m_d3dRes.m_SRVHandle = srvGPUHandle;
	srvGPUHandle.Offset(handleOffset);
	srvCPUHandle.Offset(handleOffset);

	viewer->ExecuteCommandList();
	viewer->WaitForGpu();
}

SimpleTexture2D_SingleColor::SimpleTexture2D_SingleColor(const Vec3 &col)
	: m_color(col)
{
	Vec3 _col = col;
	_col.clamp(Vec3(0.0f, 0.0f, 0.0f), Vec3(1.0f, 1.0f, 1.0f));
	m_width = m_height = 4;
	m_pixelData = new UINT8[m_width * m_height * 4];

	for (UINT32 j = 0; j < m_height; j++)
	{
		for (UINT32 i = 0; i < m_width; i++)
		{
			UINT32 index = j * m_width + i;
			float a = 1.0f;

			UINT8 *baseOffset = m_pixelData + index * 4;
			*baseOffset = static_cast<UINT32>(_col.r() * 255.0f) & 0xFF;
			*(baseOffset + 1) = static_cast<UINT32>(_col.g() * 255.0f) & 0xFF;
			*(baseOffset + 2) = static_cast<UINT32>(_col.b() * 255.0f) & 0xFF;
			*(baseOffset + 3) = static_cast<UINT32>(a * 255.0f) & 0xFF;
		}
	}
}

SimpleTexture2D_SingleColor::~SimpleTexture2D_SingleColor()
{
	if (m_pixelData)
	{
		delete[] m_pixelData;
		m_pixelData = nullptr;
	}
}

SimpleTexture2D_TGAImage::SimpleTexture2D_TGAImage(const char *filePath)
{
	FileIO _file(filePath);
	_file.Load();
	
	m_width = tgaGetWidth(_file.GetBuffer());
	m_height = tgaGetHeight(_file.GetBuffer());

	m_pixelData = (UINT8 *)tgaRead(_file.GetBuffer(), TGA_READER_ABGR);
}

SimpleTexture2D_TGAImage::~SimpleTexture2D_TGAImage()
{
	if (m_pixelData)
	{
		delete[] m_pixelData;
		m_pixelData = nullptr;
	}
}

Vec3 SimpleTexture2D_TGAImage::Sample(float u, float v) const
{
	UINT32 i = UINT32(u * m_width);
	UINT32 j = UINT32((1.0f - v) * m_height - 0.001f);

	if (i < 0) i = 0;
	if (j < 0) j = 0;
	if (i > m_width - 1) i = m_width - 1;
	if (j > m_height - 1) j = m_height - 1;

	UINT32 index =  j * m_width + i;

	float r = m_pixelData[index * 4] / 255.0f;
	float g = m_pixelData[index * 4 + 1] / 255.0f;
	float b = m_pixelData[index * 4 + 2] / 255.0f;

	return Vec3(r, g, b);
}

#pragma once

#include "Vec3.h"

class D3D12Viewer;

struct Texture2DD3D12Resources
{
	ComPtr<ID3D12Resource>			m_texture;
	CD3DX12_GPU_DESCRIPTOR_HANDLE	m_SRVHandle;
};

class ITexture2D
{
public:
	virtual ~ITexture2D() = default;
	virtual Vec3 Sample(float u, float v) const = 0;
	virtual void BuildD3DRes(D3D12Viewer *viewer, CD3DX12_CPU_DESCRIPTOR_HANDLE &srvCPUHandle, CD3DX12_GPU_DESCRIPTOR_HANDLE &srvGPUHandle) = 0;

	Texture2DD3D12Resources m_d3dRes;
};


class SimpleTexture2D : public ITexture2D
{
public:
	virtual void BuildD3DRes(D3D12Viewer *viewer, CD3DX12_CPU_DESCRIPTOR_HANDLE &srvCPUHandle, CD3DX12_GPU_DESCRIPTOR_HANDLE &srvGPUHandle) override;

	UINT32 m_width{ 0 };
	UINT32 m_height{ 0 };
	UINT8 *m_pixelData{ nullptr };
};

class SimpleTexture2D_SingleColor : public SimpleTexture2D
{
public:
	SimpleTexture2D_SingleColor(const Vec3 &col);
	virtual ~SimpleTexture2D_SingleColor() override;
	virtual Vec3 Sample(float u, float v) const override { return m_color; }
	Vec3 m_color;
};

class SimpleTexture2D_TGAImage : public SimpleTexture2D
{
public:
	SimpleTexture2D_TGAImage(const char *filePath);
	virtual ~SimpleTexture2D_TGAImage() override;
	virtual Vec3 Sample(float u, float v) const override;
};
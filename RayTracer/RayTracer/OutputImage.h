#pragma once

struct Vec3;
class D3D12Viewer;
struct PipelineState;

//RGBA8 bitmap
class OutputImage
{
public:
	OutputImage(UINT32 width, UINT32 height, const char *name);
	~OutputImage();

	void										RenderAsRainbow();
	void										RenderAsRed();
	void										Render(const Vec3 *pixels, UINT32 pixelCount = 0);

	void										Upload(D3D12Viewer *viewer);
	void										Resolve(D3D12Viewer *viewer);
	void										BuildD3DRes(D3D12Viewer *viewer);

	void										Output();

	UINT32										m_width{ 0 };
	UINT32										m_height{ 0 };
	float										m_aspectRatio{ 0.0f };
	UINT64										m_dataSizeInByte{ 0 };
	UINT32										m_pixelSizeInByte{ 4 };
	UINT8 *										m_data{ nullptr };
	std::string									m_name{};
	BOOL										m_isDirty{ FALSE };


	// for output image resolving
	PipelineState *								m_resolvePipelineState{ nullptr };
	D3D12_VERTEX_BUFFER_VIEW					m_vertexBufferView;
	ComPtr<ID3D12Resource>						m_vertexBuffer;
	ComPtr<ID3D12Resource>						m_resolveTargetTexture;
	ComPtr<ID3D12Resource>						m_resolveTargetTextureUploadHeap;
	ComPtr<ID3D12DescriptorHeap>				m_resolveTargetTextureSRVHeap;
};
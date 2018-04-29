#pragma once

class OutputImage;

class D3D12Viewer
{
public:
	D3D12Viewer(HWND hwnd, UINT32 width, UINT32 height, const OutputImage *outputImage);
	~D3D12Viewer();

	void										OnInit();
	void										OnUpdate();
	void										OnRender();
	void										OnDestroy();
	void										OnKeyDown(UINT8 keyCode);
	void										OnKeyUp(UINT8 keyCode);

private:
	void										LoadPipeline();
	void										LoadAssets();
	void										PopulateCommandList();
	void										WaitForGpu();
	void										MoveToNextFrame();

private:
	static const UINT32							FrameCount = 3;

	HWND										m_hwnd;
	UINT32										m_frameIndex{ 0 };
	UINT32										m_width{ 1028 };
	UINT32										m_height{ 720 };
	float										m_aspectRatio;
	const OutputImage *							m_image;

	CD3DX12_VIEWPORT							m_viewport;
	CD3DX12_RECT								m_scissorRect;
	ComPtr<ID3D12Device>						m_device;
	ComPtr<ID3D12CommandQueue>					m_commandQueue;
	ComPtr<IDXGISwapChain3>						m_swapChain;
	ComPtr<ID3D12Resource>						m_renderTargets[FrameCount];
	ComPtr<ID3D12CommandAllocator>				m_commandAllocators[FrameCount];
	ComPtr<ID3D12RootSignature>					m_rootSignature;
	ComPtr<ID3D12PipelineState>					m_pipelineState;
	ComPtr<ID3D12GraphicsCommandList>			m_commandList;
	ComPtr<ID3D12DescriptorHeap>				m_rtvHeap;
	UINT32										m_rtvDescriptorSize{ 0 };
	ComPtr<ID3D12DescriptorHeap>				m_srvHeap;
	ComPtr<ID3D12Resource>						m_texture;
	ComPtr<ID3D12Resource>						m_textureUploadHeap;
	UINT64										m_uploadBufferSize{ 0 };

	HANDLE										m_fenceEvent;
	ComPtr<ID3D12Fence>							m_fence;
	UINT64										m_fenceValues[FrameCount];

	// vertexs.. TODO remove
	struct Vertex
	{
		XMFLOAT3 position;
		XMFLOAT2 uv;
	};
	D3D12_VERTEX_BUFFER_VIEW					m_vertexBufferView;
	ComPtr<ID3D12Resource>						m_vertexBuffer;
};
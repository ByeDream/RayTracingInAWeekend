#pragma once

class D3D12Viewer
{
public:
	D3D12Viewer(HWND hwnd, unsigned width, unsigned height);
	~D3D12Viewer();

	void										OnInit();
	void										OnUpdate();
	void										OnRender();
	void										OnDestroy();
	void										OnKeyDown(unsigned char keyCode);
	void										OnKeyUp(unsigned char keyCode);

private:
	void										LoadPipeline();
	void										LoadAssets();
	void										PopulateCommandList();
	void										WaitForPreviousFrame();

private:
	static const unsigned						FrameCount = 3;

	HWND										m_hwnd;
	unsigned									m_frameIndex{ 0 };
	unsigned									m_width{ 1028 };
	unsigned									m_height{ 720 };
	float										m_aspectRatio;

	CD3DX12_VIEWPORT							m_viewport;
	CD3DX12_RECT								m_scissorRect;
	ComPtr<ID3D12Device>						m_device;
	ComPtr<ID3D12CommandQueue>					m_commandQueue;
	ComPtr<IDXGISwapChain3>						m_swapChain;
	ComPtr<ID3D12Resource>						m_renderTargets[FrameCount];
	ComPtr<ID3D12CommandAllocator>				m_commandAllocator;
	ComPtr<ID3D12RootSignature>					m_rootSignature;
	ComPtr<ID3D12PipelineState>					m_pipelineState;
	ComPtr<ID3D12GraphicsCommandList>			m_commandList;
	ComPtr<ID3D12DescriptorHeap>				m_rtvHeap;
	unsigned									m_rtvDescriptorSize{ 0 };

	HANDLE										m_fenceEvent;
	ComPtr<ID3D12Fence>							m_fence;
	UINT64										m_fenceValue;

	// vertexs.. TODO remove
	struct Vertex
	{
		XMFLOAT3 position;
		XMFLOAT4 color;
	};
	D3D12_VERTEX_BUFFER_VIEW					m_vertexBufferView;
	ComPtr<ID3D12Resource>						m_vertexBuffer;
};
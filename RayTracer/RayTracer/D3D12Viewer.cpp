#include "stdafx.h"
#include "D3D12Viewer.h"
#include "OutputImage.h"
#include "InputListener.h"
#include "HomemadeRayTracer.h"
#include "World.h"
#include "D3D12Helper.h"

using namespace std;

D3D12Viewer::D3D12Viewer(HWND hwnd, OutputImage *outputImage, InputListener *inputListener, HomemadeRayTracer *HMRayTracer, World *world)
	: m_hwnd(hwnd)
	, m_width(outputImage->m_width)
	, m_height(outputImage->m_height)
	, m_viewport(0.0f, 0.0f, static_cast<float>(outputImage->m_width), static_cast<float>(outputImage->m_height))
	, m_scissorRect(0, 0, static_cast<LONG>(outputImage->m_width), static_cast<LONG>(outputImage->m_height))
	, m_fenceValues{}
	, m_image(outputImage)
	, m_inputListener(inputListener)
	, m_HMRayTracer(HMRayTracer)
	, m_world(world)
{

}

D3D12Viewer::~D3D12Viewer()
{

}

void D3D12Viewer::HelpInfo()
{
	cout << "================D3D12Viewer===============" << endl;
	cout << "[Hot keys]" << endl;
	cout << "  [h] Display this message." << endl;
	cout << "  [o] Save output image to PPM file." << endl;
	cout << "  Scene viewer mode:" << endl;
	cout << "    [i] Switch to image viewer mode." << endl;
	cout << "  Image viewer mode:" << endl;
	cout << "    [esc] Switch back to Scene viewer mode." << endl;
	cout << "[Current Mode] " << D3D12ViewerModeNames[m_mode] << endl;
	cout << "==========================================" << endl;
}

void D3D12Viewer::OnInit()
{
	cout << "[D3D12Viewer] Init" << endl;
	LoadPipeline();
	LoadAssets();

	m_inputListener->RegisterKey(VK_ESCAPE);
	m_inputListener->RegisterKey('O');
	m_inputListener->RegisterKey('I');
	m_inputListener->RegisterKey('H');
}

void D3D12Viewer::OnUpdate()
{
	if (m_inputListener->WhenReleaseKey('O'))
	{
		m_image->Output();
	}

	if (m_inputListener->WhenReleaseKey('H'))
	{
		HelpInfo();
	}

	// mode switch
	if (m_mode != VMODE_SCENE_VIEWER)
	{
		if (m_inputListener->WhenReleaseKey(VK_ESCAPE))
		{
			SwitchMode(VMODE_SCENE_VIEWER);
		}
	}
	else
	{
		// auto switch to yo image viewer when image is dirty
		if (m_inputListener->WhenReleaseKey('I') || m_image->m_isDirty)
		{
			SwitchMode(VMODE_IMAGE_VIEWER);
		}
		// TODO :add more
	}
}

void D3D12Viewer::OnRender()
{
	BeginDraw();
	{
		if (m_image->m_isDirty)
		{
			UploadImage();
		}
		BeginBackSurface(TRUE); // with clear
		{
			switch (m_mode)
			{
			case VMODE_IMAGE_VIEWER:
				ResolveImage();
				break;
			case VMODE_SCENE_VIEWER:
				RenderWorld();
				break;
			default:
				// TODO
				break;
			}
		}
		EndBackSurface();
	}
	EndDraw();

	m_image->m_isDirty = FALSE;
}

void D3D12Viewer::OnDestroy()
{
	cout << "[D3D12Viewer] Destroy" << endl;
	// Ensure that the GPU is no longer referencing resources that are about to be
	// cleaned up by the destructor.
	WaitForGpu();

	CloseHandle(m_fenceEvent);
}

void D3D12Viewer::LoadPipeline()
{
	cout << "[D3D12Viewer] LoadPipeline" << endl;

	UINT32 dxgiFactoryFlags = 0;
#if defined(_DEBUG)
	{
		ComPtr<ID3D12Debug> debugController;
		if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController))))
		{
			debugController->EnableDebugLayer();

			// Enable additional debug layers.
			dxgiFactoryFlags |= DXGI_CREATE_FACTORY_DEBUG;
		}
	}
#endif

	ComPtr<IDXGIFactory4> factory;
	ThrowIfFailed(CreateDXGIFactory2(dxgiFactoryFlags, IID_PPV_ARGS(&factory)));
	ComPtr<IDXGIAdapter1> hardwareAdapter;
	GetHardwareAdapter(factory.Get(), &hardwareAdapter);

	ThrowIfFailed(D3D12CreateDevice(
		hardwareAdapter.Get(),
		D3D_FEATURE_LEVEL_11_0,
		IID_PPV_ARGS(&m_device)
	));
	NAME_D3D12_OBJECT(m_device);

	// Describe and create the command queue.
	D3D12_COMMAND_QUEUE_DESC queueDesc = {};
	queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
	queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
	ThrowIfFailed(m_device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&m_commandQueue)));
	NAME_D3D12_OBJECT(m_commandQueue);

	// Describe and create the swap chain.
	DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
	swapChainDesc.BufferCount = FrameCount;
	swapChainDesc.Width = m_width;
	swapChainDesc.Height = m_height;
	swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
	swapChainDesc.SampleDesc.Count = 1;

	ComPtr<IDXGISwapChain1> swapChain;
	ThrowIfFailed(factory->CreateSwapChainForHwnd(
		m_commandQueue.Get(),		// Swap chain needs the queue so that it can force a flush on it.
		m_hwnd,
		&swapChainDesc,
		nullptr,
		nullptr,
		&swapChain
	));

	// does not support fullscreen transitions.
	ThrowIfFailed(factory->MakeWindowAssociation(m_hwnd, DXGI_MWA_NO_ALT_ENTER));

	ThrowIfFailed(swapChain.As(&m_swapChain));
	m_frameIndex = m_swapChain->GetCurrentBackBufferIndex();


	// Create descriptor heaps.
	{
		// Describe and create a render target view (RTV) descriptor heap.
		D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc = {};
		rtvHeapDesc.NumDescriptors = FrameCount;
		rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
		rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
		ThrowIfFailed(m_device->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&m_rtvHeap)));
		NAME_D3D12_OBJECT(m_rtvHeap);

		m_rtvDescriptorSize = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

		// Describe and create a depth stencil view (DSV) descriptor heap.
		D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc = {};
		dsvHeapDesc.NumDescriptors = 1;
		dsvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
		dsvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
		ThrowIfFailed(m_device->CreateDescriptorHeap(&dsvHeapDesc, IID_PPV_ARGS(&m_dsvHeap)));
		NAME_D3D12_OBJECT(m_dsvHeap);
	}

	// Create frame resources.
	{
		CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(m_rtvHeap->GetCPUDescriptorHandleForHeapStart());

		// Create a RTV for each frame.
		for (UINT n = 0; n < FrameCount; n++)
		{
			ThrowIfFailed(m_swapChain->GetBuffer(n, IID_PPV_ARGS(&m_renderTargets[n])));
			NAME_D3D12_OBJECT_INDEXED(m_renderTargets, n);
			m_device->CreateRenderTargetView(m_renderTargets[n].Get(), nullptr, rtvHandle);
			rtvHandle.Offset(1, m_rtvDescriptorSize);
		
			ThrowIfFailed(m_device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&m_commandAllocators[n])));
			NAME_D3D12_OBJECT_INDEXED(m_commandAllocators, n);
		}
	}
}

void D3D12Viewer::LoadAssets()
{
	cout << "[D3D12Viewer] LoadAssets" << endl;

	// This is the highest version the sample supports. If CheckFeatureSupport succeeds, the HighestVersion returned will not be greater than this.
	m_RSFeatureData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_1;

	if (FAILED(m_device->CheckFeatureSupport(D3D12_FEATURE_ROOT_SIGNATURE, &m_RSFeatureData, sizeof(m_RSFeatureData))))
	{
		m_RSFeatureData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_0;
	}

	// Create the depth stencil view.
	{
		D3D12_DEPTH_STENCIL_VIEW_DESC depthStencilDesc = {};
		depthStencilDesc.Format = DXGI_FORMAT_D32_FLOAT;
		depthStencilDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
		depthStencilDesc.Flags = D3D12_DSV_FLAG_NONE;

		D3D12_CLEAR_VALUE depthOptimizedClearValue = {};
		depthOptimizedClearValue.Format = DXGI_FORMAT_D32_FLOAT;
		depthOptimizedClearValue.DepthStencil.Depth = 1.0f;
		depthOptimizedClearValue.DepthStencil.Stencil = 0;

		ThrowIfFailed(m_device->CreateCommittedResource(
			&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
			D3D12_HEAP_FLAG_NONE,
			&CD3DX12_RESOURCE_DESC::Tex2D(DXGI_FORMAT_D32_FLOAT, m_width, m_height, 1, 0, 1, 0, D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL),
			D3D12_RESOURCE_STATE_DEPTH_WRITE,
			&depthOptimizedClearValue,
			IID_PPV_ARGS(&m_depthStencil)
		));
		NAME_D3D12_OBJECT(m_depthStencil);

		m_device->CreateDepthStencilView(m_depthStencil.Get(), &depthStencilDesc, m_dsvHeap->GetCPUDescriptorHandleForHeapStart());
	}

	// Create the command list.
	ThrowIfFailed(m_device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, m_commandAllocators[m_frameIndex].Get(), nullptr, IID_PPV_ARGS(&m_commandList)));
	NAME_D3D12_OBJECT(m_commandList);
	m_commandList->Close();

	// Create synchronization objects and wait until assets have been uploaded to the GPU.
	{
		ThrowIfFailed(m_device->CreateFence(m_fenceValues[m_frameIndex], D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_fence)));
		NAME_D3D12_OBJECT(m_fence);
		m_fenceValues[m_frameIndex] = 1;

		// Create an event handle to use for frame synchronization.
		m_fenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
		if (m_fenceEvent == nullptr)
		{
			ThrowIfFailed(HRESULT_FROM_WIN32(GetLastError()));
		}
	}

	m_world->BuildD3DRes(this);
	m_image->BuildD3DRes(this);
}

void D3D12Viewer::ResetCommandList(ID3D12PipelineState *initialPSO /*= nullptr*/)
{
	ThrowIfFailed(m_commandList->Reset(m_commandAllocators[m_frameIndex].Get(), initialPSO));
}

void D3D12Viewer::ExecuteCommandList()
{
	ThrowIfFailed(m_commandList->Close());
	ID3D12CommandList* ppCommandLists[] = { m_commandList.Get() };
	m_commandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);
}

void D3D12Viewer::WaitForGpu()
{
	// Schedule a Signal command in the queue.
	ThrowIfFailed(m_commandQueue->Signal(m_fence.Get(), m_fenceValues[m_frameIndex]));

	// Wait until the fence has been processed.
	ThrowIfFailed(m_fence->SetEventOnCompletion(m_fenceValues[m_frameIndex], m_fenceEvent));
	WaitForSingleObjectEx(m_fenceEvent, INFINITE, FALSE);

	// Increment the fence value for the current frame.
	m_fenceValues[m_frameIndex]++;
}

PipelineState * D3D12Viewer::CreatePipelineState(const CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC &RSDesc, LPCWSTR VSFile, LPCWSTR PSFile, const D3D12_INPUT_LAYOUT_DESC &inputLayout, BOOL ccw, BOOL depthTest, BOOL depthWrite, BOOL alphaBlend)
{
	PipelineState *pso = new PipelineState;
	// Create the root signature.
	ComPtr<ID3DBlob> signature;
	ComPtr<ID3DBlob> error;
	ThrowIfFailed(D3DX12SerializeVersionedRootSignature(&RSDesc, m_RSFeatureData.HighestVersion, &signature, &error));
	ThrowIfFailed(m_device->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&pso->m_RS)));
	pso->m_RS->SetName(L"RootSignature");

	// Create the pipeline state, which includes compiling and loading shaders.
	ComPtr<ID3DBlob> vertexShader;
	ComPtr<ID3DBlob> pixelShader;

#if defined(_DEBUG)
	// Enable better shader debugging with the graphics debugging tools.
	UINT compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#else
	UINT compileFlags = 0;
#endif

	ThrowIfFailed(D3DCompileFromFile(VSFile, nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE, "VSMain", "vs_5_0", compileFlags, 0, &vertexShader, nullptr));
	ThrowIfFailed(D3DCompileFromFile(PSFile, nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE, "PSMain", "ps_5_0", compileFlags, 0, &pixelShader, nullptr));

		
	// Describe and create the graphics pipeline state object (PSO).
	D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
	psoDesc.InputLayout = inputLayout;
	psoDesc.pRootSignature = pso->m_RS.Get();
	psoDesc.VS = CD3DX12_SHADER_BYTECODE(vertexShader.Get());
	psoDesc.PS = CD3DX12_SHADER_BYTECODE(pixelShader.Get());
	psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	psoDesc.RasterizerState.FrontCounterClockwise = ccw; // used for simpleMesh generated by simpleMeshBuilder
	//psoDesc.RasterizerState.FillMode = D3D12_FILL_MODE_WIREFRAME;
	psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
	if (alphaBlend)
	{
		psoDesc.BlendState.RenderTarget[0] = D3D12_RENDER_TARGET_BLEND_DESC
		{
			TRUE,FALSE,
			D3D12_BLEND_SRC_ALPHA, D3D12_BLEND_INV_SRC_ALPHA, D3D12_BLEND_OP_ADD,
			D3D12_BLEND_ZERO, D3D12_BLEND_ONE, D3D12_BLEND_OP_ADD,
			D3D12_LOGIC_OP_NOOP,
			D3D12_COLOR_WRITE_ENABLE_ALL,
		};
	}
	psoDesc.DepthStencilState.DepthEnable = depthTest;
	psoDesc.DepthStencilState.DepthFunc = depthTest ? D3D12_COMPARISON_FUNC_LESS_EQUAL : D3D12_COMPARISON_FUNC_ALWAYS;
	psoDesc.DepthStencilState.DepthWriteMask = depthWrite ? D3D12_DEPTH_WRITE_MASK_ALL : D3D12_DEPTH_WRITE_MASK_ZERO;
	psoDesc.DepthStencilState.StencilEnable = FALSE;
	psoDesc.SampleMask = UINT_MAX;
	psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	psoDesc.NumRenderTargets = 1;
	psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
	psoDesc.DSVFormat = DXGI_FORMAT_D32_FLOAT;
	psoDesc.SampleDesc.Count = 1;
	ThrowIfFailed(m_device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&pso->m_PSO)));
	pso->m_RS->SetName(L"GraphicsPipelineState");

	return pso;
}

BOOL D3D12Viewer::CreateAndUnloadBuffer(ComPtr<ID3D12Resource> &out_bufferHeap, ComPtr<ID3D12Resource> &out_uploadHeap, const void *initData, UINT32 bufferSize)
{
	ThrowIfFailed(m_device->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(bufferSize),
		D3D12_RESOURCE_STATE_COPY_DEST,
		nullptr,
		IID_PPV_ARGS(&out_bufferHeap)));
	out_bufferHeap->SetName(L"defaultBufferHeap");

	// Create the GPU upload buffer.
	ThrowIfFailed(m_device->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(bufferSize),
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&out_uploadHeap)));
	out_bufferHeap->SetName(L"uploadBufferHeap");

	D3D12_SUBRESOURCE_DATA dataView = {};
	dataView.pData = initData;
	dataView.RowPitch = bufferSize;
	dataView.SlicePitch = dataView.RowPitch;

	UpdateSubresources(m_commandList.Get(), out_bufferHeap.Get(), out_uploadHeap.Get(), 0, 0, 1, &dataView);
	m_commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(out_bufferHeap.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER));

	return TRUE;
}

void D3D12Viewer::BeginDraw() 
{
	// Command list allocators can only be reset when the associated 
	// command lists have finished execution on the GPU; apps should use 
	// fences to determine GPU execution progress.
	ThrowIfFailed(m_commandAllocators[m_frameIndex]->Reset());
}

void D3D12Viewer::EndDraw()
{
	// Present the frame.
	ThrowIfFailed(m_swapChain->Present(1, 0));

	// Schedule a Signal command in the queue.
	const UINT64 currentFenceValue = m_fenceValues[m_frameIndex];
	ThrowIfFailed(m_commandQueue->Signal(m_fence.Get(), currentFenceValue));

	// Update the frame index.
	m_frameIndex = m_swapChain->GetCurrentBackBufferIndex();

	// If the next frame is not ready to be rendered yet, wait until it is ready.
	if (m_fence->GetCompletedValue() < m_fenceValues[m_frameIndex])
	{
		ThrowIfFailed(m_fence->SetEventOnCompletion(m_fenceValues[m_frameIndex], m_fenceEvent));
		WaitForSingleObjectEx(m_fenceEvent, INFINITE, FALSE);
	}

	// Set the fence value for the next frame.
	m_fenceValues[m_frameIndex] = currentFenceValue + 1;
}


void D3D12Viewer::UploadImage()
{
	ResetCommandList();

	m_image->Upload(this);

	ExecuteCommandList();
}

void D3D12Viewer::ResolveImage()
{
	ResetCommandList();

	CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle1(m_rtvHeap->GetCPUDescriptorHandleForHeapStart(), m_frameIndex, m_rtvDescriptorSize);
	m_commandList->OMSetRenderTargets(1, &rtvHandle1, FALSE, nullptr);
	m_commandList->RSSetViewports(1, &m_viewport);
	m_commandList->RSSetScissorRects(1, &m_scissorRect);

	m_image->Resolve(this);

	ExecuteCommandList();
}

void D3D12Viewer::RenderWorld()
{
	ResetCommandList();

	CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle1(m_rtvHeap->GetCPUDescriptorHandleForHeapStart(), m_frameIndex, m_rtvDescriptorSize);
	CD3DX12_CPU_DESCRIPTOR_HANDLE dsvHandle(m_dsvHeap->GetCPUDescriptorHandleForHeapStart());
	m_commandList->OMSetRenderTargets(1, &rtvHandle1, FALSE, &dsvHandle);
	m_commandList->RSSetViewports(1, &m_viewport);
	m_commandList->RSSetScissorRects(1, &m_scissorRect);

	m_world->OnRender(this);

	ExecuteCommandList();
}

void D3D12Viewer::BeginBackSurface(BOOL clear)
{
	ResetCommandList();

	// Indicate that the back buffer will be used as a render target.
	m_commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(m_renderTargets[m_frameIndex].Get(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET));

	if (clear)
	{
		CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(m_rtvHeap->GetCPUDescriptorHandleForHeapStart(), m_frameIndex, m_rtvDescriptorSize);
		CD3DX12_CPU_DESCRIPTOR_HANDLE dsvHandle(m_dsvHeap->GetCPUDescriptorHandleForHeapStart());
		m_commandList->OMSetRenderTargets(1, &rtvHandle, FALSE, &dsvHandle);
		Vec3 ambient = (1.0f - World::SkyLightBlender) * Vec3(1.0f, 1.0f, 1.0f) + World::SkyLightBlender * World::SkyLight;
		const float clearColor[] = { ambient.r(), ambient.g(), ambient.b(), 1.0f };
		m_commandList->ClearRenderTargetView(rtvHandle, clearColor, 0, nullptr);
		m_commandList->ClearDepthStencilView(m_dsvHeap->GetCPUDescriptorHandleForHeapStart(), D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);
	}
	

	ExecuteCommandList();
}

void D3D12Viewer::EndBackSurface()
{
	ResetCommandList();

	// Indicate that the back buffer will now be used to present.
	m_commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(m_renderTargets[m_frameIndex].Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT));

	ExecuteCommandList();
}

void D3D12Viewer::SwitchMode(D3D12ViewerMode mode)
{
	if (mode != m_mode)
	{
		m_mode = mode;
		cout << "[D3D12Viewer][Mode] " << D3D12ViewerModeNames[m_mode] << endl;
	}
}


#include "stdafx.h"
#include "D3D12Viewer.h"
#include "OutputImage.h"
#include "InputListener.h"
#include "HomemadeRayTracer.h"
#include "SimpleCamera.h"

//Helpers
namespace
{
	inline void ThrowIfFailed(HRESULT hr)
	{
		if (FAILED(hr))
		{
			throw std::exception();
		}
	}

	void GetHardwareAdapter(_In_ IDXGIFactory2* pFactory, _Outptr_result_maybenull_ IDXGIAdapter1** ppAdapter)
	{
		ComPtr<IDXGIAdapter1> adapter;
		*ppAdapter = nullptr;

		for (UINT adapterIndex = 0; DXGI_ERROR_NOT_FOUND != pFactory->EnumAdapters1(adapterIndex, &adapter); ++adapterIndex)
		{
			DXGI_ADAPTER_DESC1 desc;
			adapter->GetDesc1(&desc);

			if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
			{
				// Don't select the Basic Render Driver adapter.
				// If you want a software adapter, pass in "/warp" on the command line.
				continue;
			}

			// Check to see if the adapter supports Direct3D 12, but don't create the
			// actual device yet.
			if (SUCCEEDED(D3D12CreateDevice(adapter.Get(), D3D_FEATURE_LEVEL_11_0, _uuidof(ID3D12Device), nullptr)))
			{
				break;
			}
		}

		*ppAdapter = adapter.Detach();
	}

	// Assign a name to the object to aid with debugging.
#if defined(_DEBUG)
	inline void SetName(ID3D12Object* pObject, LPCWSTR name)
	{
		pObject->SetName(name);
	}
	inline void SetNameIndexed(ID3D12Object* pObject, LPCWSTR name, UINT index)
	{
		WCHAR fullName[50];
		if (swprintf_s(fullName, L"%s[%u]", name, index) > 0)
		{
			pObject->SetName(fullName);
		}
	}
#else
	inline void SetName(ID3D12Object*, LPCWSTR)
	{
	}
	inline void SetNameIndexed(ID3D12Object*, LPCWSTR, UINT)
	{
	}
#endif

	// Naming helper for ComPtr<T>.
	// Assigns the name of the variable as the name of the object.
	// The indexed variant will include the index in the name of the object.
#define NAME_D3D12_OBJECT(x) SetName(x.Get(), L#x)
#define NAME_D3D12_OBJECT_INDEXED(x, n) SetNameIndexed(x[n].Get(), L#x, n)
}


using namespace std;


/////////////////////////
#include "SimpleMesh.h"
#include "SimpeMeshBuilder.h"

struct SceneConstantBuffer
{
	XMFLOAT4X4 mvp;		// Model-view-projection (MVP) matrix.
	FLOAT padding[48];
};
SceneConstantBuffer* m_pConstantBuffers;

SimpleMesh *mesh = new SimpleMesh();

ComPtr<ID3D12Resource>						meshVertexBuffer;
D3D12_VERTEX_BUFFER_VIEW					meshVertexBufferView;
ComPtr<ID3D12Resource>						meshIndexBuffer;
D3D12_INDEX_BUFFER_VIEW						meshIndexBufferView;
UINT32										meshIndexCount;

ComPtr<ID3D12RootSignature>					meshRootSignature;
ComPtr<ID3D12PipelineState>					meshPipelineState;
ComPtr<ID3D12DescriptorHeap>				meshSrvHeap;
UINT32										cbvSrvDescriptorSize;

ComPtr<ID3D12Resource>						meshCbvUploadHeap;
/////////////////////////

D3D12Viewer::D3D12Viewer(HWND hwnd, OutputImage *outputImage, InputListener *inputListener, HomemadeRayTracer *HMRayTracer)
	: m_hwnd(hwnd)
	, m_width(outputImage->m_width)
	, m_height(outputImage->m_height)
	, m_viewport(0.0f, 0.0f, static_cast<float>(outputImage->m_width), static_cast<float>(outputImage->m_height))
	, m_scissorRect(0, 0, static_cast<LONG>(outputImage->m_width), static_cast<LONG>(outputImage->m_height))
	, m_fenceValues{}
	, m_image(outputImage)
	, m_inputListener(inputListener)
	, m_HMRayTracer(HMRayTracer)
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

void D3D12Viewer::OnUpdate(SimpleCamera *camera)
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


	/////////////////////////////////////////////////////
	XMMATRIX model;
	XMFLOAT4X4 mvp;


	model = DirectX::XMMatrixTranslation(0.0f, 0.0f, 0.0f);
	// Compute the model-view-projection matrix.
	DirectX::XMStoreFloat4x4(&mvp, DirectX::XMMatrixTranspose(model * camera->GetViewMatrix() * camera->GetProjectionMatrix()));

	// Copy this matrix into the appropriate location in the upload heap subresource.
	memcpy(&m_pConstantBuffers[m_frameIndex], &mvp, sizeof(mvp));
	/////////////////////////////////////////////////////
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
			{
				ThrowIfFailed(m_commandList->Reset(m_commandAllocators[m_frameIndex].Get(), meshPipelineState.Get()));

				// Set necessary state.
				m_commandList->SetGraphicsRootSignature(meshRootSignature.Get());
				ID3D12DescriptorHeap* ppHeaps[] = { meshSrvHeap.Get() };
				m_commandList->SetDescriptorHeaps(_countof(ppHeaps), ppHeaps);

				UINT frameResourceDescriptorOffset = m_frameIndex; //(m_cityMaterialCount + 1) + (frameResourceIndex * m_cityRowCount * m_cityColumnCount);
				CD3DX12_GPU_DESCRIPTOR_HANDLE cbvSrvHandle(meshSrvHeap->GetGPUDescriptorHandleForHeapStart(), frameResourceDescriptorOffset, cbvSrvDescriptorSize);
				m_commandList->SetGraphicsRootDescriptorTable(0, cbvSrvHandle);
				m_commandList->RSSetViewports(1, &m_viewport);
				m_commandList->RSSetScissorRects(1, &m_scissorRect);


				CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle1(m_rtvHeap->GetCPUDescriptorHandleForHeapStart(), m_frameIndex, m_rtvDescriptorSize);
				m_commandList->OMSetRenderTargets(1, &rtvHandle1, FALSE, nullptr);

				// Record commands.
				m_commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
				m_commandList->IASetIndexBuffer(&meshIndexBufferView);
				m_commandList->IASetVertexBuffers(0, 1, &meshVertexBufferView);
				m_commandList->DrawIndexedInstanced(meshIndexCount, 1, 0, 0, 0);

				ExecuteCommandList();
			}
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

		// Describe and create a shader resource view (SRV) heap for the texture.
		D3D12_DESCRIPTOR_HEAP_DESC srvHeapDesc = {};
		srvHeapDesc.NumDescriptors = 1;
		srvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
		srvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
		ThrowIfFailed(m_device->CreateDescriptorHeap(&srvHeapDesc, IID_PPV_ARGS(&m_srvHeap)));
		NAME_D3D12_OBJECT(m_srvHeap);
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

	D3D12_FEATURE_DATA_ROOT_SIGNATURE featureData = {};

	// This is the highest version the sample supports. If CheckFeatureSupport succeeds, the HighestVersion returned will not be greater than this.
	featureData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_1;

	if (FAILED(m_device->CheckFeatureSupport(D3D12_FEATURE_ROOT_SIGNATURE, &featureData, sizeof(featureData))))
	{
		featureData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_0;
	}

	// Create the root signature.
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


		ComPtr<ID3DBlob> signature;
		ComPtr<ID3DBlob> error;
		ThrowIfFailed(D3DX12SerializeVersionedRootSignature(&rootSignatureDesc, featureData.HighestVersion, &signature, &error));
		ThrowIfFailed(m_device->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&m_rootSignature)));
		NAME_D3D12_OBJECT(m_rootSignature);
	}

	// Create the pipeline state, which includes compiling and loading shaders.
	{
		ComPtr<ID3DBlob> vertexShader;
		ComPtr<ID3DBlob> pixelShader;

#if defined(_DEBUG)
		// Enable better shader debugging with the graphics debugging tools.
		UINT compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#else
		UINT compileFlags = 0;
#endif

		ThrowIfFailed(D3DCompileFromFile(L"..\\Assets\\shaders.hlsl", nullptr, nullptr, "VSMain", "vs_5_0", compileFlags, 0, &vertexShader, nullptr));
		ThrowIfFailed(D3DCompileFromFile(L"..\\Assets\\shaders.hlsl", nullptr, nullptr, "PSMain", "ps_5_0", compileFlags, 0, &pixelShader, nullptr));

		// Define the vertex input layout.
		D3D12_INPUT_ELEMENT_DESC inputElementDescs[] =
		{
			{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
			{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
		};

		// Describe and create the graphics pipeline state object (PSO).
		D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
		psoDesc.InputLayout = { inputElementDescs, _countof(inputElementDescs) };
		psoDesc.pRootSignature = m_rootSignature.Get();
		psoDesc.VS = CD3DX12_SHADER_BYTECODE(vertexShader.Get());
		psoDesc.PS = CD3DX12_SHADER_BYTECODE(pixelShader.Get());
		psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
		psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
		psoDesc.DepthStencilState.DepthEnable = FALSE;
		psoDesc.DepthStencilState.StencilEnable = FALSE;
		psoDesc.SampleMask = UINT_MAX;
		psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
		psoDesc.NumRenderTargets = 1;
		psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
		psoDesc.SampleDesc.Count = 1;
		ThrowIfFailed(m_device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&m_pipelineState)));
		NAME_D3D12_OBJECT(m_pipelineState);
	}

	// Create the command list.
	ThrowIfFailed(m_device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, m_commandAllocators[m_frameIndex].Get(), m_pipelineState.Get(), IID_PPV_ARGS(&m_commandList)));
	NAME_D3D12_OBJECT(m_commandList);


	// Note: ComPtr's are CPU objects but this resource needs to stay in scope until
	// the command list that references it has finished executing on the GPU.
	// We will flush the GPU at the end of this method to ensure the resource is not
	// prematurely destroyed.
	ComPtr<ID3D12Resource> vertexUploadHeap;

	// Create the vertex buffer.
	{
		// Define the geometry for a triangle.
		//(-1, 1) -------------- ( 1, 1)
		//   |                      |
		//   |                      |
		//   |                      |
		//(-1,-1) -------------- ( 1,-1)
		Vertex triangleVertices[] =
		{
			{ { -1.0f, 3.0f, 0.0f },{ 0.0f, -1.0f } },
			{ { 3.0f, -1.0f, 0.0f },{ 2.0f, 1.0f } },
			{ { -1.0f, -1.0f, 0.0f },{ 0.0f, 1.0f } }
		};

		const UINT vertexBufferSize = sizeof(triangleVertices);

		ThrowIfFailed(m_device->CreateCommittedResource(
			&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
			D3D12_HEAP_FLAG_NONE,
			&CD3DX12_RESOURCE_DESC::Buffer(vertexBufferSize),
			D3D12_RESOURCE_STATE_COPY_DEST,
			nullptr,
			IID_PPV_ARGS(&m_vertexBuffer)));
		NAME_D3D12_OBJECT(m_vertexBuffer);

		// Create the GPU upload buffer.
		ThrowIfFailed(m_device->CreateCommittedResource(
			&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
			D3D12_HEAP_FLAG_NONE,
			&CD3DX12_RESOURCE_DESC::Buffer(vertexBufferSize),
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			IID_PPV_ARGS(&vertexUploadHeap)));
		NAME_D3D12_OBJECT(vertexUploadHeap);

		// Copy the triangle data to the vertex buffer.
		UINT8* pVertexDataBegin;
		CD3DX12_RANGE readRange(0, 0);		// We do not intend to read from this resource on the CPU.
		ThrowIfFailed(vertexUploadHeap->Map(0, &readRange, reinterpret_cast<void**>(&pVertexDataBegin)));
		memcpy(pVertexDataBegin, triangleVertices, sizeof(triangleVertices));
		vertexUploadHeap->Unmap(0, nullptr);

		D3D12_SUBRESOURCE_DATA vertexData = {};
		vertexData.pData = &triangleVertices[0];
		vertexData.RowPitch = vertexBufferSize;
		vertexData.SlicePitch = vertexData.RowPitch;

		UpdateSubresources(m_commandList.Get(), m_vertexBuffer.Get(), vertexUploadHeap.Get(), 0, 0, 1, &vertexData);
		m_commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(m_vertexBuffer.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER));

		// Initialize the vertex buffer view.
		m_vertexBufferView.BufferLocation = m_vertexBuffer->GetGPUVirtualAddress();
		m_vertexBufferView.StrideInBytes = sizeof(Vertex);
		m_vertexBufferView.SizeInBytes = vertexBufferSize;
	}

	// Create the texture.
	{
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

		ThrowIfFailed(m_device->CreateCommittedResource(
			&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
			D3D12_HEAP_FLAG_NONE,
			&textureDesc,
			D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
			nullptr,
			IID_PPV_ARGS(&m_texture)));
		NAME_D3D12_OBJECT(m_texture);

		m_uploadBufferSize = GetRequiredIntermediateSize(m_texture.Get(), 0, 1);

		// Create the GPU upload buffer.
		ThrowIfFailed(m_device->CreateCommittedResource(
			&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
			D3D12_HEAP_FLAG_NONE,
			&CD3DX12_RESOURCE_DESC::Buffer(m_uploadBufferSize),
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			IID_PPV_ARGS(&m_textureUploadHeap)));
		NAME_D3D12_OBJECT(m_textureUploadHeap);

		// Describe and create a SRV for the texture.
		D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
		srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		srvDesc.Format = textureDesc.Format;
		srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
		srvDesc.Texture2D.MipLevels = 1;
		m_device->CreateShaderResourceView(m_texture.Get(), &srvDesc, m_srvHeap->GetCPUDescriptorHandleForHeapStart());
	}

	//////////////////////////////////
	SimpeMeshBuilder::BuildSphereMesh(mesh, 1.0f, 40, 40);
	ComPtr<ID3D12Resource> meshVertexBufferUploadHeap;
	ComPtr<ID3D12Resource> meshIndexBufferUploadHeap;
	// Create the vertex buffer.
	{
		ThrowIfFailed(m_device->CreateCommittedResource(
			&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
			D3D12_HEAP_FLAG_NONE,
			&CD3DX12_RESOURCE_DESC::Buffer(mesh->m_vertexBufferSize),
			D3D12_RESOURCE_STATE_COPY_DEST,
			nullptr,
			IID_PPV_ARGS(&meshVertexBuffer)));

		ThrowIfFailed(m_device->CreateCommittedResource(
			&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
			D3D12_HEAP_FLAG_NONE,
			&CD3DX12_RESOURCE_DESC::Buffer(mesh->m_vertexBufferSize),
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			IID_PPV_ARGS(&meshVertexBufferUploadHeap)));

		NAME_D3D12_OBJECT(meshVertexBuffer);

		// Copy data to the intermediate upload heap and then schedule a copy 
		// from the upload heap to the vertex buffer.
		D3D12_SUBRESOURCE_DATA vertexData = {};
		vertexData.pData = mesh->m_vertexBuffer;
		vertexData.RowPitch = mesh->m_vertexBufferSize;
		vertexData.SlicePitch = vertexData.RowPitch;

		UpdateSubresources<1>(m_commandList.Get(), meshVertexBuffer.Get(), meshVertexBufferUploadHeap.Get(), 0, 0, 1, &vertexData);
		m_commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(meshVertexBuffer.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER));

		// Initialize the vertex buffer view.
		meshVertexBufferView.BufferLocation = meshVertexBuffer->GetGPUVirtualAddress();
		meshVertexBufferView.StrideInBytes = mesh->m_vertexStride;
		meshVertexBufferView.SizeInBytes = mesh->m_vertexBufferSize;
	}

	// Create the index buffer.
	{
		ThrowIfFailed(m_device->CreateCommittedResource(
			&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
			D3D12_HEAP_FLAG_NONE,
			&CD3DX12_RESOURCE_DESC::Buffer(mesh->m_indexBufferSize),
			D3D12_RESOURCE_STATE_COPY_DEST,
			nullptr,
			IID_PPV_ARGS(&meshIndexBuffer)));

		ThrowIfFailed(m_device->CreateCommittedResource(
			&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
			D3D12_HEAP_FLAG_NONE,
			&CD3DX12_RESOURCE_DESC::Buffer(mesh->m_indexBufferSize),
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			IID_PPV_ARGS(&meshIndexBufferUploadHeap)));

		NAME_D3D12_OBJECT(meshIndexBuffer);

		// Copy data to the intermediate upload heap and then schedule a copy 
		// from the upload heap to the index buffer.
		D3D12_SUBRESOURCE_DATA indexData = {};
		indexData.pData = mesh->m_indexBuffer;
		indexData.RowPitch = mesh->m_indexBufferSize;
		indexData.SlicePitch = indexData.RowPitch;

		UpdateSubresources<1>(m_commandList.Get(), meshIndexBuffer.Get(), meshIndexBufferUploadHeap.Get(), 0, 0, 1, &indexData);
		m_commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(meshIndexBuffer.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_INDEX_BUFFER));

		// Describe the index buffer view.
		meshIndexBufferView.BufferLocation = meshIndexBuffer->GetGPUVirtualAddress();
		meshIndexBufferView.Format = (mesh->m_indexType == kIndexSize16) ? DXGI_FORMAT_R16_UINT : DXGI_FORMAT_R32_UINT;
		meshIndexBufferView.SizeInBytes = mesh->m_indexBufferSize;

		meshIndexCount = mesh->m_indexCount;
	}

	// Create the root signature.
	{
		CD3DX12_DESCRIPTOR_RANGE1 ranges[1];
		ranges[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 0, 0, D3D12_DESCRIPTOR_RANGE_FLAG_DATA_STATIC);

		CD3DX12_ROOT_PARAMETER1 rootParameters[1];
		rootParameters[0].InitAsDescriptorTable(1, &ranges[0], D3D12_SHADER_VISIBILITY_VERTEX);

		CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC rootSignatureDesc;
		rootSignatureDesc.Init_1_1(_countof(rootParameters), rootParameters, 0, nullptr, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);


		ComPtr<ID3DBlob> signature;
		ComPtr<ID3DBlob> error;
		ThrowIfFailed(D3DX12SerializeVersionedRootSignature(&rootSignatureDesc, featureData.HighestVersion, &signature, &error));
		ThrowIfFailed(m_device->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&meshRootSignature)));
		NAME_D3D12_OBJECT(meshRootSignature);
	}

	// Create the pipeline state, which includes compiling and loading shaders.
	{
		ComPtr<ID3DBlob> vertexShader;
		ComPtr<ID3DBlob> pixelShader;

#if defined(_DEBUG)
		// Enable better shader debugging with the graphics debugging tools.
		UINT compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#else
		UINT compileFlags = 0;
#endif

		ThrowIfFailed(D3DCompileFromFile(L"..\\Assets\\meshShaders.hlsl", nullptr, nullptr, "VSMain", "vs_5_0", compileFlags, 0, &vertexShader, nullptr));
		ThrowIfFailed(D3DCompileFromFile(L"..\\Assets\\meshShaders.hlsl", nullptr, nullptr, "PSMain", "ps_5_0", compileFlags, 0, &pixelShader, nullptr));

		struct SimpleMeshVertex
		{
			XMFLOAT3 m_position;
			XMFLOAT3 m_normal;
			XMFLOAT4 m_tangent;
			XMFLOAT2 m_texture;
		};

		// Define the vertex input layout.
		D3D12_INPUT_ELEMENT_DESC inputElementDescs[] =
		{
			{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0,  D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
			{ "NORMAL",   0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
			{ "TANGENT",  0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 24, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
			{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,    0, 40, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		};

		// Describe and create the graphics pipeline state object (PSO).
		D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
		psoDesc.InputLayout = { inputElementDescs, _countof(inputElementDescs) };
		psoDesc.pRootSignature = meshRootSignature.Get();
		psoDesc.VS = CD3DX12_SHADER_BYTECODE(vertexShader.Get());
		psoDesc.PS = CD3DX12_SHADER_BYTECODE(pixelShader.Get());
		psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
		psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
		psoDesc.DepthStencilState.DepthEnable = FALSE;
		psoDesc.DepthStencilState.StencilEnable = FALSE;
		psoDesc.SampleMask = UINT_MAX;
		psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
		psoDesc.NumRenderTargets = 1;
		psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
		psoDesc.SampleDesc.Count = 1;
		ThrowIfFailed(m_device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&meshPipelineState)));
		NAME_D3D12_OBJECT(meshPipelineState);
	}

	{
		// Describe and create a shader resource view (SRV) heap for the texture.
		D3D12_DESCRIPTOR_HEAP_DESC srvHeapDesc = {};
		srvHeapDesc.NumDescriptors = FrameCount;
		srvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
		srvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
		ThrowIfFailed(m_device->CreateDescriptorHeap(&srvHeapDesc, IID_PPV_ARGS(&meshSrvHeap)));
		NAME_D3D12_OBJECT(meshSrvHeap);

		cbvSrvDescriptorSize = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	}

	{
		// Create an upload heap for the constant buffers.
		ThrowIfFailed(m_device->CreateCommittedResource(
			&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
			D3D12_HEAP_FLAG_NONE,
			&CD3DX12_RESOURCE_DESC::Buffer(sizeof(SceneConstantBuffer) * FrameCount),
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			IID_PPV_ARGS(&meshCbvUploadHeap)));

		// Map the constant buffers. Note that unlike D3D11, the resource 
		// does not need to be unmapped for use by the GPU. In this sample, 
		// the resource stays 'permenantly' mapped to avoid overhead with 
		// mapping/unmapping each frame.
		CD3DX12_RANGE readRange(0, 0);		// We do not intend to read from this resource on the CPU.
		ThrowIfFailed(meshCbvUploadHeap->Map(0, &readRange, reinterpret_cast<void**>(&m_pConstantBuffers)));

		{
			CD3DX12_CPU_DESCRIPTOR_HANDLE cbvHandle(meshSrvHeap->GetCPUDescriptorHandleForHeapStart());

			// Create a RTV for each frame.
			UINT64 cbOffset = 0;
			for (UINT n = 0; n < FrameCount; n++)
			{
				// Describe and create a constant buffer view (CBV).
				D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc = {};
				cbvDesc.BufferLocation = meshCbvUploadHeap->GetGPUVirtualAddress() + cbOffset;
				cbvDesc.SizeInBytes = sizeof(SceneConstantBuffer);
				cbOffset += cbvDesc.SizeInBytes;
				m_device->CreateConstantBufferView(&cbvDesc, cbvHandle);
				cbvHandle.Offset(cbvSrvDescriptorSize);
			}
		}
	}
	//////////////////////////////////




	// Close the command list and execute it to begin the initial GPU setup.
	ExecuteCommandList();

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

		// Wait for the command list to execute; we are reusing the same command 
		// list in our main loop but for now, we just want to wait for setup to 
		// complete before continuing.
		WaitForGpu();
	}
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
	ThrowIfFailed(m_commandList->Reset(m_commandAllocators[m_frameIndex].Get(), nullptr));

	// Copy data to the intermediate upload heap and then schedule a copy
	// from the upload heap to the Texture2D.
	m_commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(m_texture.Get(), D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_COPY_DEST));

	D3D12_SUBRESOURCE_DATA textureData = {};
	textureData.pData = m_image->m_data;
	textureData.RowPitch = m_image->m_width * m_image->m_pixelSizeInByte;
	textureData.SlicePitch = textureData.RowPitch * m_image->m_height;

	UpdateSubresources(m_commandList.Get(), m_texture.Get(), m_textureUploadHeap.Get(), 0, 0, 1, &textureData);
	m_commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(m_texture.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE));


	ExecuteCommandList();
}

void D3D12Viewer::ResolveImage()
{
	ThrowIfFailed(m_commandList->Reset(m_commandAllocators[m_frameIndex].Get(), m_pipelineState.Get()));

	// Set necessary state.
	m_commandList->SetGraphicsRootSignature(m_rootSignature.Get());
	ID3D12DescriptorHeap* ppHeaps[] = { m_srvHeap.Get() };
	m_commandList->SetDescriptorHeaps(_countof(ppHeaps), ppHeaps);

	m_commandList->SetGraphicsRootDescriptorTable(0, m_srvHeap->GetGPUDescriptorHandleForHeapStart());
	m_commandList->RSSetViewports(1, &m_viewport);
	m_commandList->RSSetScissorRects(1, &m_scissorRect);


	CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle1(m_rtvHeap->GetCPUDescriptorHandleForHeapStart(), m_frameIndex, m_rtvDescriptorSize);
	m_commandList->OMSetRenderTargets(1, &rtvHandle1, FALSE, nullptr);

	// Record commands.
	m_commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	m_commandList->IASetVertexBuffers(0, 1, &m_vertexBufferView);
	m_commandList->DrawInstanced(3, 1, 0, 0);

	ExecuteCommandList();
}

void D3D12Viewer::BeginBackSurface(BOOL clear)
{
	ThrowIfFailed(m_commandList->Reset(m_commandAllocators[m_frameIndex].Get(), nullptr));

	// Indicate that the back buffer will be used as a render target.
	m_commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(m_renderTargets[m_frameIndex].Get(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET));

	if (clear)
	{
		CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(m_rtvHeap->GetCPUDescriptorHandleForHeapStart(), m_frameIndex, m_rtvDescriptorSize);
		m_commandList->OMSetRenderTargets(1, &rtvHandle, FALSE, nullptr);

		const float clearColor[] = { 0.0f, 0.2f, 0.4f, 1.0f };
		m_commandList->ClearRenderTargetView(rtvHandle, clearColor, 0, nullptr);
	}
	

	ExecuteCommandList();
}

void D3D12Viewer::EndBackSurface()
{
	ThrowIfFailed(m_commandList->Reset(m_commandAllocators[m_frameIndex].Get(), nullptr));

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


#include "stdafx.h"
#include "World.h"
#include "SimpleMesh.h"
#include "SimpeMeshBuilder.h"
#include "SimpleObject.h"
#include "Hitables.h"
#include "Randomizer.h"
#include "D3D12Viewer.h"
#include "D3D12Helper.h"
#include "SimpleCamera.h"
#include "SimpleTexture2D.h"

using namespace std;

const Vec3 World::SkyLight(0.5f, 0.7f, 1.0f);

const float World::SkyLightBlender = 0.3f;

void World::ConstructWorld()
{
	cout << "[World] ConstructWorld" << endl;

	LoadMeshes();
	LoadMaterials();

	std::vector<Object *> objects;

	// ground
	objects.push_back(new SimpleObjectSphere(Vec3(0.0f, -1000.0f, -0.0f), 1000.0f, m_meshes[MESH_ID_HIGH_POLYGON_SPHERE], m_materials[MATERIAL_ID_GROUND], this));

	// bigger spheres
	objects.push_back(new SimpleObjectSphere(Vec3(-4.0f, 1.0f, 0.0f), 1.0f, m_meshes[MESH_ID_MEDIUM_POLYGON_SPHERE], m_materials[MATERIAL_ID_LAMBERTIAN], this));
	objects.push_back(new SimpleObjectSphere(Vec3(0.0f, 1.0f, 0.0f), 1.0f, m_meshes[MESH_ID_MEDIUM_POLYGON_SPHERE], m_materials[MATERIAL_ID_DIELECTRIC], this));
	objects.push_back(new SimpleObjectSphere(Vec3(4.0f, 1.0f, 0.0f), 1.0f, m_meshes[MESH_ID_MEDIUM_POLYGON_SPHERE], m_materials[MATERIAL_ID_METAL], this));

	// random smaller spheres
#if 1
	for (int a = -11; a < 11; a++)
	{
		for (int b = -11; b < 11; b++)
		{
			float chooseMat = Randomizer::RandomUNorm();
			Vec3 center(a + 0.9f * Randomizer::RandomUNorm(), 0.2f, b + 0.9f * Randomizer::RandomUNorm());
			if ((center - Vec3(4.0f, 0.2f, 0.0f)).length() > 0.9f)
			{
				MaterialUniqueID materialID = MATERIAL_ID_DIELECTRIC;
				if (chooseMat < 0.8f) 
				{
					//diffuse
					materialID = (MaterialUniqueID)(UINT32)(MATERIAL_ID_RANDOM_LAMBERTIAN_START + Randomizer::RandomUNorm() * MATERIAL_ID_RANDOM_LAMBERTIAN_COUNT);
				}
				else if (chooseMat < 0.95)
				{
					// metal
					materialID = (MaterialUniqueID)(UINT32)(MATERIAL_ID_RANDOM_METAL_START + Randomizer::RandomUNorm() * MATERIAL_ID_RANDOM_METAL_COUNT);
				}
				
				objects.push_back(new SimpleObjectSphere(center, 0.2f, m_meshes[MESH_ID_LOW_POLYGON_SPHERE], m_materials[materialID], this));
			}
		}
	}
#endif

	m_objectsCount = objects.size();
	m_objectBVHTree = new SimpleObjectBVHNode(objects);
}


void World::DeconstructWorld()
{
	cout << "[World] DeconstructWorld" << endl;


	for (auto i = 0; i < MID_COUNT; ++i)
	{
		if (m_pipelineStates[i])
		{
			delete m_pipelineStates[i];
			m_pipelineStates[i] = nullptr;
		}
	}

	if (m_objectBVHTree)
	{
		delete m_objectBVHTree;
		m_objectBVHTree = nullptr;
	}

	for (auto i = m_materials.begin(); i != m_materials.end(); i++)
	{
		if ((*i) != nullptr)
		{
			delete (*i);
		}
	}
	
	for (auto i = m_textures.begin(); i != m_textures.end(); i++)
	{
		if ((*i) != nullptr)
		{
			delete (*i);
		}
	}

	for (auto i = m_meshes.begin(); i != m_meshes.end(); i++)
	{
		if ((*i) != nullptr)
		{
			delete (*i);
		}
	}
}

void World::OnUpdate(SimpleCamera *camera, float elapsedSeconds)
{
	m_CurrentCbvIndex = (m_CurrentCbvIndex + 1) % D3D12Viewer::FrameCount;

	////////////////////////
	// TODO Light, put them here at the moment
	const float lightIntensityScale = 0.6f;  // cloudy sky
	const float ambientIntensityScale = 0.4f;	// cloudy sky

	XMVECTOR lightDirWorld = DirectX::XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);  // sun light
	XMVECTOR lightDirView = DirectX::XMVector4Transform(lightDirWorld, camera->GetViewMatrix());

	Vec3 ambient = (1.0f - SkyLightBlender) * Vec3(1.0f, 1.0f, 1.0f) + SkyLightBlender * SkyLight;

	IllumConstants constants;
	DirectX::XMStoreFloat4(&constants.lightDirV, lightDirView);
	DirectX::XMStoreFloat4(&constants.lightIntensity, (SkyLight * lightIntensityScale).m_simd);
	DirectX::XMStoreFloat4(&constants.ambientIntensity, (ambient * ambientIntensityScale).m_simd);
	memcpy(m_pIllumConstants + m_IllumConstantBufferSize * m_CurrentCbvIndex, &constants, sizeof(IllumConstants));
	////////////////////////

	m_objectBVHTree->Update(camera, elapsedSeconds);
}

void World::OnRender(D3D12Viewer *viewer) const
{
	ID3D12GraphicsCommandList *commandList = viewer->GetGraphicsCommandList();
	ID3D12DescriptorHeap* ppHeaps[] = { m_SRVHeap.Get() };
	commandList->SetDescriptorHeaps(_countof(ppHeaps), ppHeaps);

	for (UINT32 mid = 0; mid < MID_COUNT; ++mid)
	{
		commandList->SetPipelineState(m_pipelineStates[mid]->m_PSO.Get());
		commandList->SetGraphicsRootSignature(m_pipelineStates[mid]->m_RS.Get());
		m_objectBVHTree->Render(viewer, mid);
	}
}

void World::BuildD3DRes(D3D12Viewer *viewer)
{
	for (auto i = m_meshes.begin(); i != m_meshes.end(); i++)
	{
		(*i)->BuildD3DRes(viewer);
	}

	{
		// create SRV heap for textures and constant buffers
		D3D12_DESCRIPTOR_HEAP_DESC srvHeapDesc = {};
		srvHeapDesc.NumDescriptors = D3D12Viewer::FrameCount * 2 * (UINT32)m_objectsCount + D3D12Viewer::FrameCount;  // Constants : big enough for use, FrameCount * geo + FrameCount *mtl per objects
		srvHeapDesc.NumDescriptors += (UINT32)m_textures.size();
		srvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
		srvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
		ThrowIfFailed(viewer->GetDevice()->CreateDescriptorHeap(&srvHeapDesc, IID_PPV_ARGS(&m_SRVHeap)));
		m_SRVHeap->SetName(L"WorldSRVHeap");
		m_CurrentCbvIndex = 0;
		CD3DX12_CPU_DESCRIPTOR_HANDLE CPUHandle(m_SRVHeap->GetCPUDescriptorHandleForHeapStart());
		CD3DX12_GPU_DESCRIPTOR_HANDLE GPUHandle(m_SRVHeap->GetGPUDescriptorHandleForHeapStart());

		m_objectBVHTree->BuildD3DRes(viewer, CPUHandle, GPUHandle);

		////////////////////////
		// TODO Light, put them here at the moment
		// illum constants
		{
			ID3D12Device *device = viewer->GetDevice();
			UINT32 handleOffset = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

			m_IllumConstantBufferSize = (sizeof(IllumConstants) + 255) & ~255;	// CB size is required to be 256-byte aligned.

																				// Create an upload heap for the Illum constant buffers.
			ThrowIfFailed(device->CreateCommittedResource(
				&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
				D3D12_HEAP_FLAG_NONE,
				&CD3DX12_RESOURCE_DESC::Buffer(m_IllumConstantBufferSize * D3D12Viewer::FrameCount),
				D3D12_RESOURCE_STATE_GENERIC_READ,
				nullptr,
				IID_PPV_ARGS(&m_IllumConstantBuffer)));
			NAME_D3D12_OBJECT(m_IllumConstantBuffer);

			// Map the constant buffers. Note that unlike D3D11, the resource 
			// does not need to be unmapped for use by the GPU. In this sample, 
			// the resource stays 'permenantly' mapped to avoid overhead with 
			// mapping/unmapping each frame.
			CD3DX12_RANGE readRange(0, 0);		// We do not intend to read from this resource on the CPU.
			ThrowIfFailed(m_IllumConstantBuffer->Map(0, &readRange, reinterpret_cast<void**>(&m_pIllumConstants)));

			// Create a CBV for each frame.
			UINT64 cbOffset = 0;
			m_IllumCbvHandles = new CD3DX12_GPU_DESCRIPTOR_HANDLE[D3D12Viewer::FrameCount];  // Note leak at rge moment!
			for (UINT n = 0; n < D3D12Viewer::FrameCount; n++)
			{
				// Describe and create a constant buffer view (CBV).
				D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc = {};
				cbvDesc.BufferLocation = m_IllumConstantBuffer->GetGPUVirtualAddress() + cbOffset;
				cbvDesc.SizeInBytes = m_IllumConstantBufferSize;
				cbOffset += m_IllumConstantBufferSize;
				device->CreateConstantBufferView(&cbvDesc, CPUHandle);
				m_IllumCbvHandles[n] = GPUHandle;
				GPUHandle.Offset(handleOffset);
				CPUHandle.Offset(handleOffset);
			}
		}
		////////////////////////


		for (auto i = m_textures.begin(); i != m_textures.end(); i++)
		{
			(*i)->BuildD3DRes(viewer, CPUHandle, GPUHandle);
		}
	}

	// create pso
	D3D12_INPUT_LAYOUT_DESC inputLayout{ SimpleMesh::D3DVertexDeclaration, SimpleMesh::D3DVertexDeclarationElementCount };

	{
		CD3DX12_DESCRIPTOR_RANGE1 ranges[4];
		ranges[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 0, 0, D3D12_DESCRIPTOR_RANGE_FLAG_DATA_STATIC);
		ranges[1].Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 1, 0, D3D12_DESCRIPTOR_RANGE_FLAG_DATA_STATIC);
		ranges[2].Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 2, 0, D3D12_DESCRIPTOR_RANGE_FLAG_DATA_STATIC);
		ranges[3].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0, 0, D3D12_DESCRIPTOR_RANGE_FLAG_DATA_STATIC);

		CD3DX12_ROOT_PARAMETER1 rootParameters[4];
		rootParameters[0].InitAsDescriptorTable(1, &ranges[0], D3D12_SHADER_VISIBILITY_VERTEX);
		rootParameters[1].InitAsDescriptorTable(1, &ranges[1], D3D12_SHADER_VISIBILITY_PIXEL);
		rootParameters[2].InitAsDescriptorTable(1, &ranges[2], D3D12_SHADER_VISIBILITY_PIXEL);
		rootParameters[3].InitAsDescriptorTable(1, &ranges[3], D3D12_SHADER_VISIBILITY_PIXEL);

		D3D12_STATIC_SAMPLER_DESC sampler = {};
		sampler.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
		sampler.AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
		sampler.AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
		sampler.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
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

		m_pipelineStates[MID_LAMBERTIAN] = viewer->CreatePipelineState(rootSignatureDesc, L"..\\Assets\\sceneGeometry_vs.hlsl", L"..\\Assets\\lambertian.hlsl", inputLayout, TRUE, TRUE, TRUE, FALSE);

		m_pipelineStates[MID_METAL] = viewer->CreatePipelineState(rootSignatureDesc, L"..\\Assets\\sceneGeometry_vs.hlsl", L"..\\Assets\\metal.hlsl", inputLayout, TRUE, TRUE, TRUE, FALSE);
	}

	{
		CD3DX12_DESCRIPTOR_RANGE1 ranges[3];
		ranges[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 0, 0, D3D12_DESCRIPTOR_RANGE_FLAG_DATA_STATIC);
		ranges[1].Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 1, 0, D3D12_DESCRIPTOR_RANGE_FLAG_DATA_STATIC);
		ranges[2].Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 2, 0, D3D12_DESCRIPTOR_RANGE_FLAG_DATA_STATIC);

		CD3DX12_ROOT_PARAMETER1 rootParameters[3];
		rootParameters[0].InitAsDescriptorTable(1, &ranges[0], D3D12_SHADER_VISIBILITY_VERTEX);
		rootParameters[1].InitAsDescriptorTable(1, &ranges[1], D3D12_SHADER_VISIBILITY_PIXEL);
		rootParameters[2].InitAsDescriptorTable(1, &ranges[2], D3D12_SHADER_VISIBILITY_PIXEL);

		CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC rootSignatureDesc;
		rootSignatureDesc.Init_1_1(_countof(rootParameters), rootParameters, 0, nullptr, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

		m_pipelineStates[MID_DIELECTRIC] = viewer->CreatePipelineState(rootSignatureDesc, L"..\\Assets\\sceneGeometry_vs.hlsl", L"..\\Assets\\dielectric.hlsl", inputLayout, TRUE, TRUE, FALSE, TRUE);
	}
}

void World::LoadMeshes()
{
	// MESH_ID_HIGH_POLYGON_SPHERE
	SimpleMesh *highPolygonSphere = new SimpleMesh();
	SimpeMeshBuilder::BuildSphereMesh(highPolygonSphere, 1.0f, 200, 200);
	m_meshes.push_back(highPolygonSphere);

	// MESH_ID_MEDIUM_POLYGON_SPHERE
	SimpleMesh *mediumPolygonSphere = new SimpleMesh();
	SimpeMeshBuilder::BuildSphereMesh(mediumPolygonSphere, 1.0f, 40, 40);
	m_meshes.push_back(mediumPolygonSphere);

	// MESH_ID_LOW_POLYGON_SPHERE
	SimpleMesh *lowPolygonSphere = new SimpleMesh();
	SimpeMeshBuilder::BuildSphereMesh(lowPolygonSphere, 1.0f, 20, 20);
	m_meshes.push_back(lowPolygonSphere);
}

void World::LoadMaterials()
{
	IMaterial *material;
	ITexture2D *texture;
	for (auto i = 0; i < MATERIAL_ID_RANDOM_LAMBERTIAN_COUNT; i++)
	{
		texture = new SimpleTexture2D_SingleColor(Vec3(Randomizer::RandomUNorm() * Randomizer::RandomUNorm(), Randomizer::RandomUNorm() * Randomizer::RandomUNorm(), Randomizer::RandomUNorm()* Randomizer::RandomUNorm()));
		m_textures.push_back(texture);
		material = new Lambertian(texture);
		m_materials.push_back(material);
	}

	for (auto i = 0; i < MATERIAL_ID_RANDOM_METAL_COUNT; i++)
	{
		texture = new SimpleTexture2D_SingleColor(Vec3(0.5f * (1 + Randomizer::RandomUNorm()), 0.5f * (1 + Randomizer::RandomUNorm()), 0.5f * (1 + Randomizer::RandomUNorm())));
		m_textures.push_back(texture);
		material = new Metal(texture, 0.5f * Randomizer::RandomUNorm());
		m_materials.push_back(material);
	}

	// MATERIAL_ID_GROUND
	texture = new SimpleTexture2D_SingleColor(Vec3(0.5f, 0.5f, 0.5f));
	m_textures.push_back(texture);
	material = new Lambertian(texture);
	m_materials.push_back(material);

	// MATERIAL_ID_LAMBERTIAN
	texture = new SimpleTexture2D_SingleColor(Vec3(0.4f, 0.2f, 0.1f));
	m_textures.push_back(texture);
	material = new Lambertian(texture);
	m_materials.push_back(material);

	// MATERIAL_ID_METAL
	texture = new SimpleTexture2D_SingleColor(Vec3(0.7f, 0.6f, 0.5f));
	m_textures.push_back(texture);
	material = new Metal(texture, 0.0f);
	m_materials.push_back(material);

	// MATERIAL_ID_DIELECTRIC
	material = new Dielectric(1.5f);
	m_materials.push_back(material);
}

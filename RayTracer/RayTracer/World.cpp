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
#include "LightSources.h"
#include "Materials.h"

using namespace std;

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
					//lambertian
					materialID = (MaterialUniqueID)(UINT32)(MATERIAL_ID_RANDOM_LAMBERTIAN_START + Randomizer::RandomUNorm() * MATERIAL_ID_RANDOM_LAMBERTIAN_COUNT);
				}
				else if (chooseMat < 0.95f)
				{
					// metal
					materialID = (MaterialUniqueID)(UINT32)(MATERIAL_ID_RANDOM_METAL_START + Randomizer::RandomUNorm() * MATERIAL_ID_RANDOM_METAL_COUNT);
				}
				
				objects.push_back(new SimpleObjectSphere(center, 0.2f, m_meshes[MESH_ID_LOW_POLYGON_SPHERE], m_materials[materialID], this));
			}
		}
	}
#endif


	m_lightSources = new LightSources(this, objects, Vec3(0.85f, 0.9f, 1.0f));
	//m_lightSources = new LightSources(this, objects, Vec3(0.0f, 0.0f, 0.0f));

	m_objectsCount = objects.size();
	m_objectBVHTree = new SimpleObjectBVHNode(objects);
}


void World::DeconstructWorld()
{
	cout << "[World] DeconstructWorld" << endl;

	if (m_objectBVHTree)
	{
		delete m_objectBVHTree;
		m_objectBVHTree = nullptr;
	}

	if (m_lightSources)
	{
		delete m_lightSources;
		m_lightSources = nullptr;
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
	
	////////////////////////

	m_lightSources->Update(camera, elapsedSeconds);

	m_objectBVHTree->Update(camera, elapsedSeconds);
}

void World::OnRender(D3D12Viewer *viewer) const
{
	ID3D12GraphicsCommandList *commandList = viewer->GetGraphicsCommandList();
	ID3D12DescriptorHeap* ppHeaps[] = { m_SRVHeap.Get() };
	commandList->SetDescriptorHeaps(_countof(ppHeaps), ppHeaps);

	DiffuseLight::ApplyPSO(viewer);
	m_objectBVHTree->Render(viewer, DiffuseLight::GetStaticID());

	Lambertian::ApplyPSO(viewer);
	m_objectBVHTree->Render(viewer, Lambertian::GetStaticID());

	Metal::ApplyPSO(viewer);
	m_objectBVHTree->Render(viewer, Metal::GetStaticID());

	Dielectric::ApplyPSO(viewer);
	m_objectBVHTree->Render(viewer, Dielectric::GetStaticID());
}

void World::BuildD3DRes(D3D12Viewer *viewer)
{
	// create SRV heap for textures and constant buffers
	D3D12_DESCRIPTOR_HEAP_DESC srvHeapDesc = {};
	// cbv :
	// geo constants: frameCount * objectCount
	srvHeapDesc.NumDescriptors = D3D12Viewer::FrameCount * (UINT32)m_objectsCount;
	// mtl constants: materialCount
	srvHeapDesc.NumDescriptors += (UINT32)m_materials.size();
	// illum constants: frameCount * (1 + lightSourceCount)
	srvHeapDesc.NumDescriptors += D3D12Viewer::FrameCount * (1 + m_lightSources->GetLightSourceCount());
	// textures srv
	srvHeapDesc.NumDescriptors += (UINT32)m_textures.size();
	srvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	srvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	ThrowIfFailed(viewer->GetDevice()->CreateDescriptorHeap(&srvHeapDesc, IID_PPV_ARGS(&m_SRVHeap)));
	m_SRVHeap->SetName(L"WorldSRVHeap");
	m_CurrentCbvIndex = 0;

	CD3DX12_CPU_DESCRIPTOR_HANDLE CPUHandle(m_SRVHeap->GetCPUDescriptorHandleForHeapStart());
	CD3DX12_GPU_DESCRIPTOR_HANDLE GPUHandle(m_SRVHeap->GetGPUDescriptorHandleForHeapStart());

	m_objectBVHTree->BuildD3DRes(viewer, CPUHandle, GPUHandle);

	m_lightSources->BuildD3DRes(viewer, CPUHandle, GPUHandle);

	for (auto i = m_textures.begin(); i != m_textures.end(); i++)
	{
		(*i)->BuildD3DRes(viewer, CPUHandle, GPUHandle);
	}

	for (auto i = m_materials.begin(); i != m_materials.end(); i++)
	{
		(*i)->BuildD3DRes(viewer, CPUHandle, GPUHandle);
	}

	for (auto i = m_meshes.begin(); i != m_meshes.end(); i++)
	{
		(*i)->BuildD3DRes(viewer);
	}

	// create pso
	DiffuseLight::BuildPSO(viewer);
	Lambertian::BuildPSO(viewer, m_lightSources->GetLightSourceCount());
	Metal::BuildPSO(viewer, m_lightSources->GetLightSourceCount());
	Dielectric::BuildPSO(viewer, m_lightSources->GetLightSourceCount());
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
	//texture = new SimpleTexture2D_TGAImage("..\\Assets\\pab_ground_soil_001_c.tga");
	//m_textures.push_back(texture);

	texture = new SimpleTexture2D_SingleColor(Vec3(0.4f, 0.2f, 0.1f));
	m_textures.push_back(texture);
	material = new Lambertian(texture);
	//material = new DiffuseLight(Vec3(4.0f, 4.0f, 4.0f));
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

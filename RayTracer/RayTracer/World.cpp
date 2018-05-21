#include "stdafx.h"
#include "World.h"
#include "SimpleObject.h"
#include "Hitables.h"
#include "Randomizer.h"
#include "D3D12Viewer.h"
#include "D3D12Helper.h"
#include "SimpleCamera.h"
#include "LightSources.h"
#include "Resouces.h"
#include "Materials.h"

using namespace std;

void World::ConstructWorld(WorldID wid, SimpleCamera *camera)
{
	cout << "[World] ConstructWorld" << endl;
	m_resources = new Resources();
	m_resources->Load();

	std::vector<Object *> objects;

	switch (wid)
	{
	case WORLD_ID_RANDOM_SPHERES:
	{
		// ground
		objects.push_back(new SimpleObjectSphere(1000.0f, m_resources->GetTheMesh(MESH_ID_HIGH_POLYGON_SPHERE), m_resources->GetTheMaterial(MATERIAL_ID_LAMBERTIAN0), this, Vec3(0.0f, -1000.0f, -0.0f)));

		// bigger spheres
		objects.push_back(new SimpleObjectSphere(1.0f, m_resources->GetTheMesh(MESH_ID_MEDIUM_POLYGON_SPHERE), m_resources->GetTheMaterial(MATERIAL_ID_LAMBERTIAN1), this, Vec3(-4.0f, 1.0f, 0.0f)));
		objects.push_back(new SimpleObjectSphere(1.0f, m_resources->GetTheMesh(MESH_ID_MEDIUM_POLYGON_SPHERE), m_resources->GetTheMaterial(MATERIAL_ID_DIELECTRIC), this, Vec3(0.0f, 1.0f, 0.0f)));
		objects.push_back(new SimpleObjectSphere(1.0f, m_resources->GetTheMesh(MESH_ID_MEDIUM_POLYGON_SPHERE), m_resources->GetTheMaterial(MATERIAL_ID_METAL), this, Vec3(4.0f, 1.0f, 0.0f)));

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

					objects.push_back(new SimpleObjectSphere(0.2f, m_resources->GetTheMesh(MESH_ID_LOW_POLYGON_SPHERE), m_resources->GetTheMaterial(materialID), this, center));
				}
			}
		}
#endif

		m_lightSources = new LightSources(this, objects, Vec3(0.85f, 0.9f, 1.0f));
		camera->Initialize(Vec3(11.0f, 2.0f, 3.0f), Vec3(0.0f, 0.6f, 0.0f), 20.0f, 1.0f, 10000.0f, 0.1f, 10.0f, 1.0f);

		break;
	}

	case WORLD_ID_CORNELL_BOX:
	{
		// light on the top
		objects.push_back(new SimpleObjectRect(XZ_RECT, Vec3(0.0f, 0.99f, 0.0f), 0.65f, 0.65f, TRUE, m_resources->GetTheMesh(MESH_ID_QUAD), m_resources->GetTheMaterial(MATERIAL_ID_LIGHTSOURCE_BRIGHT), this));

		// red wall at the right
		objects.push_back(new SimpleObjectRect(ZY_RECT, Vec3(1.0f, 0.0f, 0.0f), 2.0f, 2.0f, TRUE, m_resources->GetTheMesh(MESH_ID_QUAD), m_resources->GetTheMaterial(MATERIAL_ID_LAMBERTIAN2), this));
		// green wall at the left
		objects.push_back(new SimpleObjectRect(ZY_RECT, Vec3(-1.0f, 0.0f, 0.0f), 2.0f, 2.0f, FALSE, m_resources->GetTheMesh(MESH_ID_QUAD), m_resources->GetTheMaterial(MATERIAL_ID_LAMBERTIAN4), this));
		// white floor, background and ceiling
		objects.push_back(new SimpleObjectRect(XY_RECT, Vec3(0.0f, 0.0f, -1.0f), 2.0f, 2.0f, FALSE, m_resources->GetTheMesh(MESH_ID_QUAD), m_resources->GetTheMaterial(MATERIAL_ID_LAMBERTIAN3), this));
		objects.push_back(new SimpleObjectRect(XZ_RECT, Vec3(0.0f, 1.0f, 0.0f), 2.0f, 2.0f, TRUE, m_resources->GetTheMesh(MESH_ID_QUAD), m_resources->GetTheMaterial(MATERIAL_ID_LAMBERTIAN3), this));
		objects.push_back(new SimpleObjectRect(XZ_RECT, Vec3(0.0f, -1.0f, 0.0f), 2.0f, 2.0f, FALSE, m_resources->GetTheMesh(MESH_ID_QUAD), m_resources->GetTheMaterial(MATERIAL_ID_LAMBERTIAN3), this));

		float height = 1.1f;
		objects.push_back(new SimpleObjectCube(Vec3(-0.3f, -1.0f + height * 0.5f, 0.0f), Vec3(0.6f, height, 0.4f), m_resources->GetTheMesh(MESH_ID_CUBE), m_resources->GetTheMaterial(MATERIAL_ID_LAMBERTIAN1), this));

		m_lightSources = new LightSources(this, objects, Vec3(0.0f, 0.0f, 0.0f));
		camera->Initialize(Vec3(0.0f, 0.0f, 4.0f), Vec3(0.0f, 0.0f, 0.0f), 40.0f, 1.0f, 10000.0f, 0.0f, 10.0f, 1.0f);

		break;
	}
	default:
		assert(false);
		break;
	}

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

	if (m_resources)
	{
		m_resources->Unload();
		delete m_resources;
		m_resources = nullptr;
	}
}

void World::OnUpdate(SimpleCamera *camera, float elapsedSeconds)
{
	m_CurrentCbvIndex = (m_CurrentCbvIndex + 1) % D3D12Viewer::FrameCount;

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
	{
		D3D12_DESCRIPTOR_HEAP_DESC srvHeapDesc = {};
		// cbv :
		// geo constants: frameCount * objectCount
		srvHeapDesc.NumDescriptors = D3D12Viewer::FrameCount * (UINT32)m_objectsCount;
		// mtl constants: materialCount
		srvHeapDesc.NumDescriptors += (UINT32)(m_resources->GetMaterialsCount());
		// illum constants: frameCount * (1 + lightSourceCount)
		srvHeapDesc.NumDescriptors += D3D12Viewer::FrameCount * (1 + m_lightSources->GetLightSourceCount());
		// textures srv
		srvHeapDesc.NumDescriptors += (UINT32)(m_resources->GetTexturesCount());
		srvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
		srvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
		ThrowIfFailed(viewer->GetDevice()->CreateDescriptorHeap(&srvHeapDesc, IID_PPV_ARGS(&m_SRVHeap)));
		m_SRVHeap->SetName(L"WorldSRVHeap");
	}
	
	m_CurrentCbvIndex = 0;
	CD3DX12_CPU_DESCRIPTOR_HANDLE CPUHandle(m_SRVHeap->GetCPUDescriptorHandleForHeapStart());
	CD3DX12_GPU_DESCRIPTOR_HANDLE GPUHandle(m_SRVHeap->GetGPUDescriptorHandleForHeapStart());

	m_objectBVHTree->BuildD3DRes(viewer, CPUHandle, GPUHandle);
	m_lightSources->BuildD3DRes(viewer, CPUHandle, GPUHandle);
	m_resources->BuildD3DRes(viewer, CPUHandle, GPUHandle);

	// create pso
	DiffuseLight::BuildPSO(viewer);
	Lambertian::BuildPSO(viewer, m_lightSources->GetLightSourceCount());
	Metal::BuildPSO(viewer, m_lightSources->GetLightSourceCount());
	Dielectric::BuildPSO(viewer, m_lightSources->GetLightSourceCount());
}
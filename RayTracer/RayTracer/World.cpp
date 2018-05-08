#include "stdafx.h"
#include "World.h"
#include "SimpleMesh.h"
#include "SimpeMeshBuilder.h"
#include "Materials.h"
#include "SimpleObject.h"
#include "Hitables.h"
#include "Randomizer.h"
#include "D3D12Viewer.h"

using namespace std;

void World::ConstructWorld()
{
	cout << "[World] ConstructWorld" << endl;

	LoadMeshes();
	LoadMaterials();

	// ground
	m_lambertianObjects.push_back(new SimpleSphereObject(Vec3(0.0f, -1000.0f, -0.0f), 1000.0f, m_meshes[MESH_ID_HIGH_POLYGON_SPHERE], m_materials[MATERIAL_ID_GROUND]));

	// bigger spheres
	m_lambertianObjects.push_back(new SimpleSphereObject(Vec3(-4.0f, 1.0f, 0.0f), 1.0f, m_meshes[MESH_ID_HIGH_POLYGON_SPHERE], m_materials[MATERIAL_ID_LAMBERTIAN]));
	m_dielectricObjects.push_back(new SimpleSphereObject(Vec3(0.0f, 1.0f, 0.0f), 1.0f, m_meshes[MESH_ID_HIGH_POLYGON_SPHERE], m_materials[MATERIAL_ID_DIELECTRIC]));
	m_metalObjects.push_back(new SimpleSphereObject(Vec3(4.0f, 1.0f, 0.0f), 1.0f, m_meshes[MESH_ID_HIGH_POLYGON_SPHERE], m_materials[MATERIAL_ID_METAL]));

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
				std::vector<Object *> *objectPool = &m_dielectricObjects;
				if (chooseMat < 0.8f) 
				{
					//diffuse
					materialID = (MaterialUniqueID)(UINT32)(MATERIAL_ID_RANDOM_LAMBERTIAN_START + Randomizer::RandomUNorm() * MATERIAL_ID_RANDOM_LAMBERTIAN_COUNT);
					objectPool = &m_lambertianObjects;
				}
				else if (chooseMat < 0.95)
				{
					// metal
					materialID = (MaterialUniqueID)(UINT32)(MATERIAL_ID_RANDOM_METAL_START + Randomizer::RandomUNorm() * MATERIAL_ID_RANDOM_METAL_COUNT);
					objectPool = &m_metalObjects;
				}
				
				objectPool->push_back(new SimpleSphereObject(center, 0.2f, m_meshes[MESH_ID_LOW_POLYGON_SPHERE], m_materials[materialID]));
			}
		}
	}
#endif

	for (auto i = m_lambertianObjects.begin(); i != m_lambertianObjects.end(); i++)
		m_objects.push_back(*i);
	for (auto i = m_metalObjects.begin(); i != m_metalObjects.end(); i++)
		m_objects.push_back(*i);
	for (auto i = m_dielectricObjects.begin(); i != m_dielectricObjects.end(); i++)
		m_objects.push_back(*i);
}


void World::DeconstructWorld()
{
	cout << "[World] DeconstructWorld" << endl;

	if (m_lambertianPipelineState)
	{
		delete m_lambertianPipelineState;
		m_lambertianPipelineState = nullptr;
	}
	if (m_metalPipelineState)
	{
		delete m_metalPipelineState;
		m_metalPipelineState = nullptr;
	}
	if (m_dielectricPipelineState)
	{
		delete m_dielectricPipelineState;
		m_dielectricPipelineState = nullptr;
	}

	for (auto i = m_objects.begin(); i != m_objects.end(); i++)
	{
		if ((*i) != nullptr)
		{
			delete (*i);
		}
	}

	for (auto i = m_materials.begin(); i != m_materials.end(); i++)
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
	for (auto i = m_objects.begin(); i != m_objects.end(); i++)
	{
		(*i)->Update(camera, elapsedSeconds);
	}
}

void World::OnRender(D3D12Viewer *viewer) const
{
	ID3D12GraphicsCommandList *commandList = viewer->GetGraphicsCommandList();

	commandList->SetPipelineState(m_lambertianPipelineState->m_PSO.Get());
	commandList->SetGraphicsRootSignature(m_lambertianPipelineState->m_RS.Get());
	for (auto i = m_lambertianObjects.begin(); i != m_lambertianObjects.end(); i++)
	{
		(*i)->Render(viewer);
	}

	commandList->SetPipelineState(m_metalPipelineState->m_PSO.Get());
	commandList->SetGraphicsRootSignature(m_metalPipelineState->m_RS.Get());
	for (auto i = m_metalObjects.begin(); i != m_metalObjects.end(); i++)
	{
		(*i)->Render(viewer);
	}

	commandList->SetPipelineState(m_dielectricPipelineState->m_PSO.Get());
	commandList->SetGraphicsRootSignature(m_dielectricPipelineState->m_RS.Get());
	for (auto i = m_dielectricObjects.begin(); i != m_dielectricObjects.end(); i++)
	{
		(*i)->Render(viewer);
	}
}

BOOL World::Hit(const Ray &r, float t_min, float t_max, HitRecord &out_rec) const
{
	HitRecord rec;
	BOOL hitAnything = FALSE;
	float cloestSoFar = t_max;
	for (auto i = m_objects.begin(); i != m_objects.end(); i++)
	{
		if ((*i)->m_hitable->Hit(r, t_min, cloestSoFar, rec))
		{
			hitAnything = TRUE;
			cloestSoFar = rec.m_time;
			out_rec = rec;
		}
	}
	return hitAnything;
}

void World::BuildD3DRes(D3D12Viewer *viewer)
{
	for (auto i = m_meshes.begin(); i != m_meshes.end(); i++)
	{
		(*i)->BuildD3DRes(viewer);
	}

	for (auto i = m_objects.begin(); i != m_objects.end(); i++)
	{
		(*i)->BuildD3DRes(viewer);
	}

	// create pso
	CD3DX12_DESCRIPTOR_RANGE1 ranges[1];
	ranges[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 0, 0, D3D12_DESCRIPTOR_RANGE_FLAG_DATA_STATIC);

	CD3DX12_ROOT_PARAMETER1 rootParameters[1];
	rootParameters[0].InitAsDescriptorTable(1, &ranges[0], D3D12_SHADER_VISIBILITY_VERTEX);

	CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC rootSignatureDesc;
	rootSignatureDesc.Init_1_1(_countof(rootParameters), rootParameters, 0, nullptr, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

	D3D12_INPUT_LAYOUT_DESC inputLayout{ SimpleMesh::D3DVertexDeclaration, SimpleMesh::D3DVertexDeclarationElementCount };

	m_lambertianPipelineState = viewer->CreatePipelineState(rootSignatureDesc, L"..\\Assets\\meshShaders.hlsl", inputLayout);

	m_metalPipelineState = viewer->CreatePipelineState(rootSignatureDesc, L"..\\Assets\\meshShaders.hlsl", inputLayout);

	m_dielectricPipelineState = viewer->CreatePipelineState(rootSignatureDesc, L"..\\Assets\\meshShaders.hlsl", inputLayout);
}

void World::LoadMeshes()
{
	// MESH_ID_HIGH_POLYGON_SPHERE
	SimpleMesh *highPolygonSphere = new SimpleMesh();
	SimpeMeshBuilder::BuildSphereMesh(highPolygonSphere, 1.0f, 40, 40);
	m_meshes.push_back(highPolygonSphere);

	// MESH_ID_LOW_POLYGON_SPHERE
	SimpleMesh *lowPolygonSphere = new SimpleMesh();
	SimpeMeshBuilder::BuildSphereMesh(lowPolygonSphere, 1.0f, 20, 20);
	m_meshes.push_back(lowPolygonSphere);
}

void World::LoadMaterials()
{
	IMaterial *material;
	for (auto i = 0; i < MATERIAL_ID_RANDOM_LAMBERTIAN_COUNT; i++)
	{
		material = new Lambertian(Vec3(Randomizer::RandomUNorm() * Randomizer::RandomUNorm(), Randomizer::RandomUNorm() * Randomizer::RandomUNorm(), Randomizer::RandomUNorm()* Randomizer::RandomUNorm()));
		m_materials.push_back(material);
	}

	for (auto i = 0; i < MATERIAL_ID_RANDOM_METAL_COUNT; i++)
	{
		material = new Metal(Vec3(0.5f * (1 + Randomizer::RandomUNorm()), 0.5f * (1 + Randomizer::RandomUNorm()), 0.5f * (1 + Randomizer::RandomUNorm())), 0.5f * Randomizer::RandomUNorm());
		m_materials.push_back(material);
	}

	// MATERIAL_ID_GROUND
	material = new Lambertian(Vec3(0.5f, 0.5f, 0.5f));
	m_materials.push_back(material);

	// MATERIAL_ID_LAMBERTIAN
	material = new Lambertian(Vec3(0.4f, 0.2f, 0.1f));
	m_materials.push_back(material);

	// MATERIAL_ID_METAL
	material = new Metal(Vec3(0.7f, 0.6f, 0.5f), 0.0f);
	m_materials.push_back(material);

	// MATERIAL_ID_DIELECTRIC
	material = new Dielectric(1.5f);
	m_materials.push_back(material);
}

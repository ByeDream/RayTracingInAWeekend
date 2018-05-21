#include "stdafx.h"
#include "Resouces.h"

#include "Randomizer.h"
#include "SimpleMesh.h"
#include "SimpeMeshBuilder.h"
#include "SimpleTexture2D.h"
#include "Materials.h"

void Resources::Load()
{
	LoadMeshes();
	LoadMaterials();
}

void Resources::Unload()
{
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

void Resources::BuildD3DRes(D3D12Viewer *viewer, CD3DX12_CPU_DESCRIPTOR_HANDLE &CPUHandle, CD3DX12_GPU_DESCRIPTOR_HANDLE &GPUHandle)
{
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
}

Mesh *Resources::GetTheMesh(MeshUniqueID id) const
{
	assert(id >= 0 && id < m_meshes.size());
	return m_meshes[id];
}

IMaterial *Resources::GetTheMaterial(MaterialUniqueID id) const
{
	assert(id >= 0 && id < m_materials.size());
	return m_materials[id];
}

void Resources::LoadMeshes()
{
	std::cout << "[Resources] Load meshes" << std::endl;

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

void Resources::LoadMaterials()
{
	std::cout << "[Resources] Load materials and textures" << std::endl;

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

	// MATERIAL_ID_LAMBERTIAN0
	texture = new SimpleTexture2D_SingleColor(Vec3(0.5f, 0.5f, 0.5f));
	m_textures.push_back(texture);
	material = new Lambertian(texture);
	m_materials.push_back(material);

	// MATERIAL_ID_LAMBERTIAN1
	texture = new SimpleTexture2D_SingleColor(Vec3(0.4f, 0.2f, 0.1f));
	m_textures.push_back(texture);
	material = new Lambertian(texture);
	m_materials.push_back(material);

	// MATERIAL_ID_LAMBERTIAN2
	texture = new SimpleTexture2D_SingleColor(Vec3(0.65f, 0.05f, 0.05f));
	m_textures.push_back(texture);
	material = new Lambertian(texture);
	m_materials.push_back(material);

	// MATERIAL_ID_LAMBERTIAN3
	texture = new SimpleTexture2D_SingleColor(Vec3(0.73f, 0.73f, 0.73f));
	m_textures.push_back(texture);
	material = new Lambertian(texture);
	m_materials.push_back(material);

	// MATERIAL_ID_LAMBERTIAN4
	texture = new SimpleTexture2D_SingleColor(Vec3(0.12f, 0.45f, 0.15f));
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

	// MATERIAL_ID_IMAGE_BASED_GROUND_SOIL
	texture = new SimpleTexture2D_TGAImage("..\\Assets\\pab_ground_soil_001_c.tga");
	m_textures.push_back(texture);
	material = new Lambertian(texture);
	m_materials.push_back(material);

	//MATERIAL_ID_IMAGE_BASED_METAL_CHECKER
	texture = new SimpleTexture2D_TGAImage("..\\Assets\\pro_metal_checker_plate_001_c.tga");
	m_textures.push_back(texture);
	material = new Metal(texture, 0.7f);
	m_materials.push_back(material);

	// MATERIAL_ID_LIGHTSOURCE_WHITE
	material = new DiffuseLight(Vec3(4.0f, 4.0f, 4.0f));
	m_materials.push_back(material);

	// MATERIAL_ID_LIGHTSOURCE_BRIGHT
	material = new DiffuseLight(Vec3(15.0f, 15.0f, 15.0f));
	m_materials.push_back(material);

	// MATERIAL_ID_LIGHTSOURCE_RED
	material = new DiffuseLight(Vec3(4.0f, 0.0f, 0.0f));
	m_materials.push_back(material);

	// MATERIAL_ID_LIGHTSOURCE_GREEN
	material = new DiffuseLight(Vec3(0.0f, 4.0f, 0.0f));
	m_materials.push_back(material);

	// MATERIAL_ID_LIGHTSOURCE_BLUE
	material = new DiffuseLight(Vec3(0.0f, 0.0f, 4.0f));
	m_materials.push_back(material);
}
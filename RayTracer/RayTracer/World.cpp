#include "stdafx.h"
#include "World.h"
#include "SimpleMesh.h"
#include "SimpeMeshBuilder.h"
#include "Materials.h"
#include "SimpleObject.h"
#include "Hitables.h"
#include "Randomizer.h"

using namespace std;

void World::ConstructWorld()
{
	cout << "[World] ConstructWorld" << endl;

	LoadMeshes();
	LoadMaterials();

	// ground
	m_objects.push_back(new SimpleSphereObject(Vec3(0.0f, -1000.0f, -0.0f), 1000.0f, m_meshes[MESH_ID_HIGH_POLYGON_SPHERE], m_materials[MATERIAL_ID_GROUND]));

	// bigger spheres
	m_objects.push_back(new SimpleSphereObject(Vec3(0.0f, 1.0f, 0.0f), 1.0f, m_meshes[MESH_ID_HIGH_POLYGON_SPHERE], m_materials[MATERIAL_ID_DIELECTRIC]));
	m_objects.push_back(new SimpleSphereObject(Vec3(-4.0f, 1.0f, 0.0f), 1.0f, m_meshes[MESH_ID_HIGH_POLYGON_SPHERE], m_materials[MATERIAL_ID_LAMBERTIAN]));
	m_objects.push_back(new SimpleSphereObject(Vec3(4.0f, 1.0f, 0.0f), 1.0f, m_meshes[MESH_ID_HIGH_POLYGON_SPHERE], m_materials[MATERIAL_ID_METAL]));

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
				m_objects.push_back(new SimpleSphereObject(center, 0.2f, m_meshes[MESH_ID_LOW_POLYGON_SPHERE], m_materials[materialID]));

			}
		}
	}
#endif
}


void World::DeconstructWorld()
{
	cout << "[World] DeconstructWorld" << endl;

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

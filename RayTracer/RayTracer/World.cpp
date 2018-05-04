#include "stdafx.h"
#include "World.h"
#include "Materials.h"
#include "Hitables.h"

using namespace std;

void World::ConstructWorld()
{
	cout << "[World] ConstructWorld" << endl;

	//m_hitableList = new Hitable *[m_hitableListSize];
	//m_materialList = new Material *[m_materialListSize];

	Hitable *hitable = nullptr;
	Material *material = nullptr;

	Dielectric *dielectricMaterial = new Dielectric(1.5f);
	m_materialList.push_back(dielectricMaterial);

	// ground
	hitable = new SphereHitable(Vec3(0.0f, -1000.0f, -0.0f), 1000.0f);
	material = new Lambertian(Vec3(0.5f, 0.5f, 0.5f));
	hitable->BindMaterial(material);
	m_hitableList.push_back(hitable);
	m_materialList.push_back(material);

	// random small spheres
#if 0
	for (int a = -11; a < 11; a++)
	{
		for (int b = -11; b < 11; b++)
		{
			float chooseMat = Randomizer::RandomUNorm();
			Vec3 center(a + 0.9f * Randomizer::RandomUNorm(), 0.2f, b + 0.9f * Randomizer::RandomUNorm());
			if ((center - Vec3(4.0f, 0.2f, 0.0f)).length() > 0.9f)
			{
				hitable = new SphereHitable(center, 0.2f);
				m_hitableList.push_back(hitable);
				if (chooseMat < 0.8f) 
				{
					//diffuse
					material = new Lambertian(Vec3(Randomizer::RandomUNorm() * Randomizer::RandomUNorm(), Randomizer::RandomUNorm() * Randomizer::RandomUNorm(), Randomizer::RandomUNorm()* Randomizer::RandomUNorm()));
					m_materialList.push_back(material);
				}
				else if (chooseMat < 0.95)
				{
					// metal
					material = new Metal(Vec3(0.5f * (1 + Randomizer::RandomUNorm()), 0.5f * (1 + Randomizer::RandomUNorm()), 0.5f * (1 + Randomizer::RandomUNorm())), 0.5f * Randomizer::RandomUNorm());
					m_materialList.push_back(material);
				}
				else
				{
					// dielectric
					material = dielectricMaterial;
				}

				hitable->BindMaterial(material);
			}
		}
	}
#endif

	hitable = new SphereHitable(Vec3(0.0f, 1.0f, 0.0f), 1.0f);
	hitable->BindMaterial(dielectricMaterial);
	m_hitableList.push_back(hitable);

	hitable = new SphereHitable(Vec3(-4.0f, 1.0f, 0.0f), 1.0f);
	material = new Lambertian(Vec3(0.4f, 0.2f, 0.1f));
	hitable->BindMaterial(material);
	m_hitableList.push_back(hitable);
	m_materialList.push_back(material);

	hitable = new SphereHitable(Vec3(4.0f, 1.0f, 0.0f), 1.0f);
	material = new Metal(Vec3(0.7f, 0.6f, 0.5f), 0.0f);
	hitable->BindMaterial(material);
	m_hitableList.push_back(hitable);
	m_materialList.push_back(material);

	m_rootHitable = new HitableCombo(m_hitableList);
}


void World::DeconstructWorld()
{
	cout << "[World] DeconstructWorld" << endl;
	for (auto i = m_materialList.begin(); i != m_materialList.end(); i++)
	{
		if ((*i) != nullptr)
		{
			delete (*i);
		}
	}

	for (auto i = m_hitableList.begin(); i != m_hitableList.end(); i++)
	{
		if ((*i) != nullptr)
		{
			delete (*i);
		}
	}

	delete m_rootHitable;
	m_rootHitable = nullptr;
}
#include "stdafx.h"
#include "HomemadeRayTracer.h"

#include "Ray.h"
#include "Hitables.h"
#include "OutputImage.h"
#include "SimpleCamera.h"
#include "Randomizer.h"

#include "InputListener.h"
#include "Materials.h"

using namespace std;

#define SHOW_PROGRESS
#define SAMPLE_COUNT 100
#define MAX_SAMPLE_DEPTH  50

HomemadeRayTracer::HomemadeRayTracer(InputListener *inputListener, OutputImage *image)
	: m_inputListener(inputListener)
	, m_image(image)
{

}

HomemadeRayTracer::~HomemadeRayTracer()
{

}

void HomemadeRayTracer::OnInit()
{
	ConstructHitableWorld();

	Vec3 lookFrom(11.0f, 2.0f, 3.0f);
	Vec3 lookAt(0.0f, 0.6f, 0.0f);
	float focousDist = (lookFrom - lookAt).length();
	//m_camera = new SimpleCamera(Vec3(-1.0f, 0.5f, -0.3f), Vec3(0.0f, 0.0f, -1.0f), Vec3(0.0f, 1.0f, 0.0f), 90.0f, m_image->m_aspectRatio, 1.0f);
	m_camera = new SimpleCamera(lookFrom, lookAt, Vec3(0.0f, 1.0f, 0.0f), 20.0f, m_image->m_aspectRatio, 0.1f, focousDist);

	m_inputListener->RegisterKey('R');
	m_inputListener->RegisterKey('H');
	m_inputListener->RegisterKey('N');
	m_inputListener->RegisterKey('M');
}

void HomemadeRayTracer::OnUpdate()
{
	if (m_inputListener->WhenReleaseKey('R'))
	{
		Render(m_image);
	}

	if (m_inputListener->WhenReleaseKey('H'))
	{
		HelpInfo();
	}

	if (m_inputListener->WhenReleaseKey('N'))
	{
		m_enableNormalDisplay = !m_enableNormalDisplay;

		cout << "[HomemadeRayTracer][NormalDisplay] " << (m_enableNormalDisplay ? "Enabled" : "Disabled") << endl;
	}

	if (m_inputListener->WhenReleaseKey('M'))
	{
		m_enblaeAA = !m_enblaeAA;

		cout << "[HomemadeRayTracer][AA] " << (m_enblaeAA ? "Enabled" : "Disabled") << endl;
	}
}

void HomemadeRayTracer::OnDestroy()
{
	if (m_camera)
	{
		delete m_camera;
		m_camera = nullptr;
	}
	DeconstructHitableWorld();
}

void HomemadeRayTracer::HelpInfo()
{
	cout << "=============HomemadeRayTracer============" << endl;
	cout << "[Hot keys]" << endl;
	cout << "  [h] Display this message." << endl;
	cout << "  [r] Render result to output image and upload it to viewer." << endl;
	cout << "  [n] Switch on/off normal display." << endl;
	cout << "  [m] Switch on/off anti-aliasing." << endl;
	cout << "[NormalDisplay] " << (m_enableNormalDisplay ? "Enabled" : "Disabled") << endl;
	cout << "[AA] " << (m_enblaeAA ? "Enabled" : "Disabled") << endl;
	cout << "==========================================" << endl;
}

void HomemadeRayTracer::Render(OutputImage *image)
{
	cout << "[HomemadeRayTracer] Rendering ..." << endl;

	cout << "[HomemadeRayTracer] Sample count per pixel: " << SAMPLE_COUNT << endl;

	//image->RenderAsRainbow();

	UINT32 width = image->m_width;
	UINT32 height = image->m_height;

	Vec3 *pixels = new Vec3[width * height];

	INT32 nthreads, tid;
	UINT32 progress = 0;
#pragma omp parallel for default(none) shared(pixels, progress) private(nthreads, tid)
	for (INT32 j = 0; j < (INT32)height; j++) // To use omp, I have to use signed index.
	{
		for (UINT32 i = 0; i < width; i++)
		{
			Vec3 &col = pixels[j * width + i];

			if (m_enblaeAA)
			{
				// Multi-sample
				col.zero();
				for (UINT32 s = 0; s < SAMPLE_COUNT; s++)
				{
					float u = float(i + Randomizer::RandomUNorm()) / float(width);
					float v = float(j + Randomizer::RandomUNorm()) / float(height);

					Ray r = m_camera->GetRay(u, v);
					col += Sample(r, *m_hitableWorld, 0);
				}
				col /= float(SAMPLE_COUNT);

				// the gamma correction, to the approximation, use the power 1/gamma, and the gamma == 2, which is just square-root.
				col = Vec3(sqrt(col[0]), sqrt(col[1]), sqrt(col[2]));
			}
			else
			{
				float u = float(i) / float(width);
				float v = float(j) / float(height);
				Ray r = m_camera->GetRay(u, v);
				col = Sample(r, *m_hitableWorld, 0);
			}
			
		}

#if defined(SHOW_PROGRESS)
		nthreads = omp_get_num_threads();
		tid = omp_get_thread_num();
		progress++;
		printf("[Thread %02d(%d)]%.2lf%%\r", tid, nthreads, progress * 100.0 / height);
#endif
	}
	image->Render(pixels);

	// housekeeping
	delete[] pixels;

	cout << "[HomemadeRayTracer] Done" << endl;
}

Vec3 HomemadeRayTracer::Sample(const Ray &r, const Hitable &world, UINT32 depth) const
{
	Vec3 col;
	HitRecord rec;
	// Ignore hits very near 0 to get rid of the shdow acne.
	if (world.Hit(r, 0.001f, FLT_MAX, rec))
	{
		if (m_enableNormalDisplay)
		{
			col = 0.5f * Vec3(rec.m_normal.x() + 1.0f, rec.m_normal.y() + 1.0f, rec.m_normal.z() + 1.0f);
		}
		else
		{
			Vec3 attenuation;
			Ray r_scattered;
			if (depth < MAX_SAMPLE_DEPTH && rec.m_hitMaterial && rec.m_hitMaterial->Scatter(r, rec, attenuation, r_scattered))
			{
				col = attenuation * Sample(r_scattered, world, depth + 1);
			}
			else
			{
				col.zero();
			}
		}
	}
	else
	{
		// Sky light
		// blends white and blue depending on the up/downess of the y coordinate
		Vec3 dir = normalize(r.m_dir);
		float t = 0.5f * (dir.y() + 1.0f);
		// lerp
		col = (1.0f - t) * Vec3(1.0f, 1.0f, 1.0f) + t * Vec3(0.5f, 0.7f, 1.0f);
	}

	return col;
}

void HomemadeRayTracer::ConstructHitableWorld()
{
	cout << "[HomemadeRayTracer] ConstructHitableWorld" << endl;

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
#if 1
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

	m_hitableWorld = new HitableCombo(m_hitableList);

}

void HomemadeRayTracer::DeconstructHitableWorld()
{
	cout << "[HomemadeRayTracer] DeconstructHitableWorld" << endl;
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

	delete m_hitableWorld;
	m_hitableWorld = nullptr;
}
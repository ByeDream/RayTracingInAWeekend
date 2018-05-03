#include "stdafx.h"
#include "HomemadeRayTracer.h"

#include "Ray.h"
#include "Hitables.h"
#include "OutputImage.h"
#include "SimpleCamera.h"
#include "RandomFloat.h"

#include "InputListener.h"
#include "Materials.h"

using namespace std;

#define SHOW_PROGRESS
#define SAMPLE_COUNT 8
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

	m_camera = new SimpleCamera(Vec3(0.0f, 0.0f, 0.0f), Vec3(0.0f, 1.0f, 0.0f), Vec3(0.0f, 0.0f, -1.0f), m_image->m_aspectRatio, 1.0f, 3000.0f);

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
	for (UINT32 j = 0; j < height; j++)
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
					float u = float(i + Random(0.0f, 1.0f)) / float(width);
					float v = float(j + Random(0.0f, 1.0f)) / float(height);

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
		printf("%.2lf%%\r", j * 100.0 / height);
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
	m_materialListSize = 4;
	m_materialList = new Material *[m_materialListSize];
	m_materialList[0] = new Lambertian(Vec3(0.8f, 0.3f, 0.3f));
	m_materialList[1] = new Lambertian(Vec3(0.8f, 0.8f, 0.0f));
	m_materialList[2] = new Metal(Vec3(0.8f, 0.6f, 0.2f), 1.0f);
	m_materialList[3] = new Dielectric(1.5);

	//m_materialListSize = sizeof(m_materialList) / sizeof(Material *);


	m_hitableListSize = 4;
	m_hitableList = new Hitable *[m_hitableListSize];
	m_hitableList[0] = new SphereHitable(Vec3(0.0f, 0.0f, -1.0f), 0.5f);
	m_hitableList[0]->BindMaterial(m_materialList[0]);
	m_hitableList[1] = new SphereHitable(Vec3(0.0f, -100.5f, -1.0f), 100.0f);
	m_hitableList[1]->BindMaterial(m_materialList[1]);
	m_hitableList[2] = new SphereHitable(Vec3(1.0f, 0.0f, -1.0f), 0.5f);
	m_hitableList[2]->BindMaterial(m_materialList[2]);
	m_hitableList[3] = new SphereHitable(Vec3(-1.0f, 0.0f, -1.0f), 0.5f);
	m_hitableList[3]->BindMaterial(m_materialList[3]);

	//m_hitableListSize =  sizeof(m_hitableList) / sizeof(Hitable *);

	m_hitableWorld = new HitableCombo(m_hitableList, m_hitableListSize);

}

void HomemadeRayTracer::DeconstructHitableWorld()
{
	cout << "[HomemadeRayTracer] DeconstructHitableWorld" << endl;
	for (auto i = 0; i < m_hitableListSize; i++)
	{
		delete m_hitableList[i];
	}
	delete[] m_hitableList;
	m_hitableList = nullptr;

	for (auto i = 0; i < m_materialListSize; i++)
	{
		delete m_materialList[i];
	}
	delete[] m_materialList;
	m_materialList = nullptr;

	delete m_hitableWorld;
	m_hitableWorld = nullptr;
}
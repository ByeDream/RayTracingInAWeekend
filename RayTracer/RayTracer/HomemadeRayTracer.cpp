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

HomemadeRayTracer::HomemadeRayTracer(InputListener *inputListener, OutputImage *image, const Hitable *world)
	: m_inputListener(inputListener)
	, m_world(world)
{

}

HomemadeRayTracer::~HomemadeRayTracer()
{

}

void HomemadeRayTracer::OnInit()
{
	m_inputListener->RegisterKey('R');
	m_inputListener->RegisterKey('H');
	m_inputListener->RegisterKey('N');
	m_inputListener->RegisterKey('M');
}

void HomemadeRayTracer::OnUpdate(const SimpleCamera *camera, OutputImage *image)
{
	if (m_inputListener->WhenReleaseKey('R'))
	{
		Render(camera, image);
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

void HomemadeRayTracer::Render(const SimpleCamera *camera, OutputImage *image)
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

					Ray r = camera->GetRay(u, v);
					col += Sample(r, 0);
				}
				col /= float(SAMPLE_COUNT);

				// the gamma correction, to the approximation, use the power 1/gamma, and the gamma == 2, which is just square-root.
				col = Vec3(sqrt(col[0]), sqrt(col[1]), sqrt(col[2]));
			}
			else
			{
				float u = float(i) / float(width);
				float v = float(j) / float(height);
				Ray r = camera->GetRay(u, v);
				col = Sample(r, 0);
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

Vec3 HomemadeRayTracer::Sample(const Ray &r, UINT32 depth) const
{
	Vec3 col;
	HitRecord rec;
	// Ignore hits very near 0 to get rid of the shdow acne.
	if (m_world->Hit(r, 0.001f, FLT_MAX, rec))
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
				col = attenuation * Sample(r_scattered, depth + 1);
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

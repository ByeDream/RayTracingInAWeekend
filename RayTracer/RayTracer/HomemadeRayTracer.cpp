#include "stdafx.h"
#include "HomemadeRayTracer.h"

#include "Ray.h"
#include "World.h"
#include "OutputImage.h"
#include "SimpleCamera.h"
#include "Randomizer.h"

#include "InputListener.h"
#include "Materials.h"

#include "SimpleObject.h"
#include "Hitables.h"

using namespace std;

#define SHOW_PROGRESS
#define SAMPLE_PER_PIXEL 100
#define MAX_SAMPLE_DEPTH  50

HomemadeRayTracer::HomemadeRayTracer(InputListener *inputListener, OutputImage *image, const World *world)
	: m_inputListener(inputListener)
	, m_world(world)
{

}

HomemadeRayTracer::~HomemadeRayTracer()
{

}

void HomemadeRayTracer::OnInit()
{
	cout << "[HomemadeRayTracer] Init" << endl;
	m_inputListener->RegisterKey(VK_SPACE);
	m_inputListener->RegisterKey('H');
	m_inputListener->RegisterKey('N');
	m_inputListener->RegisterKey('M');
}

void HomemadeRayTracer::OnUpdate(const SimpleCamera *camera, OutputImage *image)
{
	if (m_inputListener->WhenReleaseKey(VK_SPACE))
	{
		TraceRay(camera, image);
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
		m_enable1SPP = !m_enable1SPP;

		cout << "[HomemadeRayTracer][1SPP] " << (m_enable1SPP ? "Enabled" : "Disabled") << endl;
	}
}

void HomemadeRayTracer::OnDestroy()
{
	cout << "[HomemadeRayTracer] Destroy" << endl;
}

void HomemadeRayTracer::HelpInfo()
{
	cout << "=============HomemadeRayTracer============" << endl;
	cout << "[Hot keys]" << endl;
	cout << "  [h] Display this message." << endl;
	cout << "  [space] Render result to output image and upload it to viewer." << endl;
	cout << "  [n] Switch on/off normal display." << endl;
	cout << "  [m] Switch on/off 1-SPP." << endl;
	cout << "[NormalDisplay] " << (m_enableNormalDisplay ? "Enabled" : "Disabled") << endl;
	cout << "[1SPP] " << (m_enable1SPP ? "Enabled" : "Disabled") << endl;
	cout << "==========================================" << endl;
}

void HomemadeRayTracer::TraceRay(const SimpleCamera *camera, OutputImage *image)
{
	cout << "[HomemadeRayTracer] TraceRay ..." << endl;
	if (m_enable1SPP)
		cout << "[HomemadeRayTracer] SPP: 1" << endl;
	else
		cout << "[HomemadeRayTracer] SPP: " << SAMPLE_PER_PIXEL << endl;

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

			if (m_enable1SPP)
			{
				// Single-sample
				float u = float(i) / float(width);
				float v = float(j) / float(height);
				Ray r = camera->GetRay(u, v);
				col = Sample(r, 0);
			}
			else
			{
				// Multi-sample
				col.zero();
				for (UINT32 s = 0; s < SAMPLE_PER_PIXEL; s++)
				{
					float u = float(i + Randomizer::RandomUNorm()) / float(width);
					float v = float(j + Randomizer::RandomUNorm()) / float(height);

					Ray r = camera->GetRay(u, v);
					col += Sample(r, 0);
				}
				col /= float(SAMPLE_PER_PIXEL);
			}

			// the gamma correction, to the approximation, use the power 1/gamma, and the gamma == 2, which is just square-root.
			col = Vec3(sqrt(col[0]), sqrt(col[1]), sqrt(col[2]));
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
	float nearest = 0.001f; // Ignore hits very near 0 to get rid of the shadow acne.
	float cloestSoFar = FLT_MAX;
	if (m_world->GetObjectBVHTree()->Hit(r, nearest, cloestSoFar, rec))
	{
		if (m_enableNormalDisplay)
		{
			col = 0.5f * Vec3(rec.m_normal.x() + 1.0f, rec.m_normal.y() + 1.0f, rec.m_normal.z() + 1.0f);
		}
		else
		{
			Vec3 attenuation;
			Ray r_scattered;
			Vec3 emmitted;
			emmitted.zero();
			if (depth < MAX_SAMPLE_DEPTH && rec.m_hitMaterial && rec.m_hitMaterial->Scatter(r, rec, attenuation, r_scattered, emmitted))
			{
				col = emmitted + attenuation * Sample(r_scattered, depth + 1);
			}
			else
			{
				col = emmitted;
			}
		}
	}
	else
	{
		// blends white and Sky light depending on the up/downess of the y coordinate
		Vec3 dir = normalize(r.m_dir);
		float t = 0.5f * (dir.y() + 1.0f);
		// lerp
		col = (1.0f - t) * Vec3(1.0f, 1.0f, 1.0f) + t * World::SkyLight;
	}

	return col;
}

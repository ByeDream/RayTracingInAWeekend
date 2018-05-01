#include "stdafx.h"
#include "HomemadeRayTracer.h"

#include "Ray.h"
#include "Hitables.h"
#include "OutputImage.h"
#include "SimpleCamera.h"
#include "RandomFloat.h"

using namespace std;

#define SHOW_PROGRESS
#define SampleCount 8

// TODO : remove temp code:
namespace
{
	Vec3 romdomInUnitSphere() 
	{
		Vec3 p;
		do
		{
			p = 2.0f * Vec3(Random(), Random(), Random()) - Vec3(1.0f, 1.0f, 1.0f);
		} while (p.squared_length() >= 1.0f);

		return p;
	}
}

HomemadeRayTracer::HomemadeRayTracer()
{

}

HomemadeRayTracer::~HomemadeRayTracer()
{

}

void HomemadeRayTracer::Trace(OutputImage *image)
{
	cout << "[HomemadeRayTracer] Tracing ..." << endl;

	cout << "[HomemadeRayTracer] Sample count per pixel: " << SampleCount << endl;

	//image->RenderAsRainbow();

	ConstructHitableWorld();

	SimpleCamera camera(Vec3(0.0f, 0.0f, 0.0f), Vec3(0.0f, 1.0f, 0.0f), Vec3(0.0f, 0.0f, -1.0f), image->m_aspectRatio, 1.0f, 3000.0f);

	UINT32 width = image->m_width;
	UINT32 height = image->m_height;

	Vec3 *pixels = new Vec3[width * height];
	for (UINT32 j = 0; j < height; j++)
	{
		for (UINT32 i = 0; i < width; i++)
		{
			Vec3 &col = pixels[j * width + i];
			// Multi-sample
			col.zero();
			for (UINT32 s = 0; s < SampleCount; s++)
			{
				float u = float(i + Random(0.0f, 1.0f)) / float(width);
				float v = float(j + Random(0.0f, 1.0f)) / float(height);

				Ray r = camera.GetRay(u, v);
				col += Sample(r, *m_hitableWorld);
			}
			col /= float(SampleCount);
		}

#if defined(SHOW_PROGRESS)
		printf("%.2lf%%\r", j * 100.0 / height);
#endif
	}
	image->Render(pixels);

	// housekeeping
	delete[] pixels;
	DeconstructHitableWorld();

	cout << "[HomemadeRayTracer] Done" << endl;
}

Vec3 HomemadeRayTracer::Sample(const Ray &r, const Hitable &world) const
{
	HitRecord rec;
	// TODO : instead of use 0.0f, use the t of hit point with view plane.
	if (world.Hit(r, 0.0f, FLT_MAX, rec))
	{
		Vec3 target = rec.m_position + rec.m_normal + romdomInUnitSphere();
		//return 0.5f * Vec3(rec.m_normal.x() + 1.0f, rec.m_normal.y() + 1.0f, rec.m_normal.z() + 1.0f);
		
		return 0.5f * Sample( Ray(rec.m_position, target - rec.m_position), world); 
	}
	else
	{
		// BgColor
		// blends white and blue depending on the up/downess of the y coordinate
		Vec3 dir = normalize(r.m_dir);
		float t = 0.5f * (dir.y() + 1.0f);
		// lerp
		return (1.0f - t) * Vec3(1.0f, 1.0f, 1.0f) + t * Vec3(0.5f, 0.7f, 1.0f);
	}
}

void HomemadeRayTracer::ConstructHitableWorld()
{
	cout << "[HomemadeRayTracer] ConstructHitableWorld" << endl;
	Hitable **list = new Hitable *[2];
	list[0] = new SphereHitable(Vec3(0.0f, 0.0f, -1.0f), 0.5f);
	list[1] = new SphereHitable(Vec3(0.0f, -100.5f, -1.0f), 100.0f);
	m_hitableWorld = new HitableCombo(list, 2);
}

void HomemadeRayTracer::DeconstructHitableWorld()
{
	cout << "[HomemadeRayTracer] DeconstructHitableWorld" << endl;
	HitableCombo *world = reinterpret_cast<HitableCombo *>(m_hitableWorld);
	for (UINT32 i = 0; i < world->m_arraySize; i++)
	{
		delete world->m_pointerArray[i];
	}
	delete[] world->m_pointerArray;
	delete m_hitableWorld;
	m_hitableWorld = nullptr;
}
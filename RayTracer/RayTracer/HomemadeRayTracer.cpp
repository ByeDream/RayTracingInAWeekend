#include "stdafx.h"
#include "HomemadeRayTracer.h"

#include "Ray.h"
#include "OutputImage.h"

using namespace std;

//temp code
namespace
{
	Vec3 BgColor(const Ray &r)
	{
		// blends white and blue depending on the up/downess of the y coordinate
		Vec3 dir = normalize(r.m_dir);
		float t = 0.5f * (dir.y() + 1.0f);
		// lerp
		return (1.0f - t) * Vec3(1.0f, 1.0f, 1.0f) + t * Vec3(0.5f, 0.7f, 1.0f);
	}
}

void HomemadeRayTracer::Trace(OutputImage *image)
{
	cout << "[HomemadeRayTracer] Trace ..." << endl;
	//image->RenderAsRainbow();


	const float viewSurfaceAspectRatio = static_cast<float>(image->m_width) / static_cast<float>(image->m_height);
	const float viewSurfaceHalfHeight = 1.0f;
	const float viewSurfaceHalfWidth = viewSurfaceHalfHeight * viewSurfaceAspectRatio;

	Vec3 viewSurfaceHigherLeftCorner(-viewSurfaceHalfWidth, viewSurfaceHalfHeight, -1.0f); //right hand coordinate
	Vec3 viewSurfaceHorizontal(2.0f * viewSurfaceHalfWidth, 0.0f, 0.0f);
	Vec3 viewSurfaceVertical(0.0f, -2.0f * viewSurfaceHalfHeight, 0.0f);
	Vec3 viewPos(0.0f, 0.0f, 0.0f);


	Vec3 *pixels = new Vec3[image->m_width * image->m_height];
	for (UINT32 j = 0; j < image->m_height; j++)
	{
		for (UINT32 i = 0; i < image->m_width; i++)
		{
			float u = float(i) / float(image->m_width);
			float v = float(j) / float(image->m_height);

			Ray r(viewPos, viewSurfaceHigherLeftCorner + u * viewSurfaceHorizontal + v * viewSurfaceVertical);
			pixels[j * image->m_width + i] = BgColor(r);
		}
	}
	image->Render(pixels);
	delete[] pixels;
	cout << "[HomemadeRayTracer] Done" << endl;
}
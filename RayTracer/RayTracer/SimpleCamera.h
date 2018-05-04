#pragma once

#include "Ray.h"

class SimpleCamera
{
public:
	SimpleCamera(const Vec3 &lookFrom, const Vec3 &lookAt, const Vec3 &ViewUp, float fov, float aspectRatio, float aperture, float focus_dist);

	Ray GetRay(float u, float v) const;

	Vec3 m_viewTopLeftCorner;
	Vec3 m_viewHorizontal;
	Vec3 m_viewVertical;

	Vec3 m_origin;
	Vec3 m_u;
	Vec3 m_v;
	Vec3 m_w;
	float mLensRadius;
	//Vec3 m_lookAt;
	//Vec3 m_vup;
};
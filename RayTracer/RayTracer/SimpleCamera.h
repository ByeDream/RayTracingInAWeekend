#pragma once

#include "Ray.h"

class SimpleCamera
{
public:
	SimpleCamera(const Vec3 &pos, const Vec3 &up, const Vec3 &forward, float aspectRatio, float minZ, float maxZ);

	Ray GetRay(float u, float v) const;

	Vec3 m_viewPlaneHigherLeftCorner;
	Vec3 m_viewPlaneHorizontal;
	Vec3 m_viewPlaneVertical;
	Vec3 m_origin;
};
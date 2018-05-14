#pragma once

#include "Vec3.h"

class Ray
{
public:
	Ray() = default;
	Ray(const Vec3 &org, const Vec3 &dir)
		: m_org(org)
		, m_dir(dir)
	{}

	Vec3 PointAt(float t) const { return m_org + t * m_dir; }

	Vec3 m_org;
	Vec3 m_dir;
};
#pragma once

#include "Vec3.h"

class Ray;

class AABB
{
public:
	AABB() = default;
	AABB(const Vec3 &_min, const Vec3 &_max) : m_min(_min), m_max(_max) {}

	BOOL Hit(const Ray &r, float t_min, float t_max) const;

	Vec3 m_min;
	Vec3 m_max;
};

inline AABB CombineAABB(const AABB &box0, const AABB &box1)
{
	Vec3 _min(
		min(box0.m_min.x(), box1.m_min.x()),
		min(box0.m_min.y(), box1.m_min.y()),
		min(box0.m_min.z(), box1.m_min.z())
	);

	Vec3 _max(
		max(box0.m_max.x(), box1.m_max.x()),
		max(box0.m_max.y(), box1.m_max.y()),
		max(box0.m_max.z(), box1.m_max.z())
	);

	return AABB(_min, _max);
}
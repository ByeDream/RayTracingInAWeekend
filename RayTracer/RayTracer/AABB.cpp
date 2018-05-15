#include "stdafx.h"
#include "AABB.h"
#include "Ray.h"

BOOL AABB::Hit(const Ray &r, float t_min, float t_max) const
{
	for (UINT32 axle = 0; axle < 3; axle++) // 3 a
	{
		float invD = 1.0f / r.m_dir[axle];
		float t0 = (m_min[axle] - r.m_org[axle]) * invD;
		float t1 = (m_max[axle] - r.m_org[axle]) * invD;
		// make sure t0 < t1
		if (invD < 0.0f)
			std::swap(t0, t1);
		t_min = max(t0, t_min);
		t_max = min(t1, t_max);
		if (t_max <= t_min)
			return FALSE;
	}
	return TRUE;
}

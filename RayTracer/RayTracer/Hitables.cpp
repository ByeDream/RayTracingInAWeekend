#include "stdafx.h"
#include "Hitables.h"

#include "Ray.h"

SphereHitable::SphereHitable(const Vec3 &center, float radius)
	: m_center(center)
	, m_radius(radius)
{

}

BOOL SphereHitable::Hit(const Ray &r, float t_min, float t_max, HitRecord &out_rec) const
{
	// dot(ray(t) - center, ray(t) - center) = radius * radius
	// dot(org + t * dir - center, org + t * dir - center) = radius * radius
	// dot((t * dir) + (org - center), (t * dir) + (org - center)) - radius * radius = 0
	// dot(t * dir, t * dir) + 2 * dot(t * dir, org - center) + dot(org - center, org - center) - radius * radius = 0
	// t * t * dot(dir, dir) + 2 * t * dot(dir, org - center) + dot(org - cebter, org - cebter) - radius * radius = 0
	Vec3 oc = r.m_org - m_center;
	float a = dot(r.m_dir, r.m_dir);
	float b = 2.0f * dot(r.m_dir, oc);
	float c = dot(oc, oc) - m_radius * m_radius;

	float discriminant = b * b - 4 * a * c;

	BOOL hit = FALSE;
	if (discriminant > 0)
	{
		// when discriminant == 0, there is only one solution that is -b / (2.0f * a), which means hit with the edge of Sphere
		// when discriminant > 0, it is could be +/- sqrt(discriminant), which mean hit with separate far/near point on the Sphere
		float t = (-b - sqrt(discriminant)) / (2.0f * a);
		if (t <= t_max && t >= t_min)
		{
			out_rec.m_time = t;
			out_rec.m_position = r.PointAt(t);
			out_rec.m_normal = (out_rec.m_position - m_center) / m_radius; // same as normalize, cos the length is know as m_radius
			out_rec.m_hitMaterial = m_material;
			CalculateUV(out_rec);
			return TRUE; // the nearest hitting on ray direction
		}
		t = (-b + sqrt(discriminant)) / (2.0f * a);
		if (t <= t_max && t >= t_min)
		{
			out_rec.m_time = t;
			out_rec.m_position = r.PointAt(t);
			out_rec.m_normal = (out_rec.m_position - m_center) / m_radius; // same as normalize, cos the length is know as m_radius
			out_rec.m_hitMaterial = m_material;
			CalculateUV(out_rec);
			return TRUE; // the farthest hitting on ray direction
		}
	}
	return FALSE; // no hit
}

AABB SphereHitable::BoundingBox() const
{
	Vec3 radius(m_radius, m_radius, m_radius);
	return AABB(-radius, radius);
}

void SphereHitable::CalculateUV(HitRecord &rec) const
{
	// hit point in model space
	Vec3 p = rec.m_position - m_center;
	p.normalize();

	float theta = atan2(p.z(), p.x());
	float phi = asin(p.y());

	// Sorry for trick code, but it is for matching with what we got from simple mesh builder, 
	// still right hand, but +z faces up and +x faces left
	//float theta = atan2(p.y(), -p.x());
	//float phi = asin(-p.z());

	rec.m_u = 1.0f - (theta + (float)M_PI) / (2.0f * (float)M_PI);
	rec.m_v = (phi + (float)M_PI / 2.0f) / (float)M_PI;
}

AxisAlignedRectHitable::AxisAlignedRectHitable(UINT32 aAxisIndex, UINT32 bAxisIndex, float a0, float a1, float b0, float b1, float c, BOOL reverseFace)
	: m_a0(a0), m_a1(a1), m_b0(b0), m_b1(b1), m_c(c), m_aAxisIndex(aAxisIndex), m_bAxisIndex(bAxisIndex), m_reverseFace(reverseFace)
{
	assert(m_aAxisIndex < 3 && m_bAxisIndex < 3 && m_aAxisIndex != m_bAxisIndex && "Invalid arguments");
	m_cAxisIndex = (m_aAxisIndex == 0 ? (m_bAxisIndex == 1 ? 2 : 1) : (m_aAxisIndex == 1 ? (m_bAxisIndex == 0 ? 2 : 0) : (m_bAxisIndex == 1 ? 0 : 1)));

	if (m_a0 > m_a1)
		std::swap(m_a0, m_a1);

	if (m_b0 > m_b1)
		std::swap(m_b0, m_b1);
}

BOOL AxisAlignedRectHitable::Hit(const Ray &r, float t_min, float t_max, HitRecord &out_rec) const
{
	float t = (m_c - r.m_org[m_cAxisIndex]) / r.m_dir[m_cAxisIndex];
	if (t <= t_max && t >= t_min)
	{
		float a = r.m_org[m_aAxisIndex] + t * r.m_dir[m_aAxisIndex];
		float b = r.m_org[m_bAxisIndex] + t * r.m_dir[m_bAxisIndex];
		if (a >= m_a0 && a <= m_a1 && b >= m_b0 && b <= m_b1)
		{
			out_rec.m_u = (a - m_a0) / (m_a1 - m_a0);
			out_rec.m_v = (b - m_b0) / (m_b1 - m_b0);
			out_rec.m_time = t;
			out_rec.m_hitMaterial = m_material;
			out_rec.m_position = r.PointAt(t);
			out_rec.m_normal.set(m_aAxisIndex, 0.0f);
			out_rec.m_normal.set(m_bAxisIndex, 0.0f);
			out_rec.m_normal.set(m_cAxisIndex, m_reverseFace ? -1.0f : 1.0f);
			return TRUE;
		}
	}

	return FALSE;
}

AABB AxisAlignedRectHitable::BoundingBox() const
{
	Vec3 _min, _max;
	_min.set(m_aAxisIndex, m_a0);
	_max.set(m_aAxisIndex, m_a1);
	_min.set(m_bAxisIndex, m_b0);
	_max.set(m_bAxisIndex, m_b1);
	_min.set(m_cAxisIndex, m_c - 0.0001f);
	_max.set(m_cAxisIndex, m_c + 0.0001f);

	return AABB(_min, _max);
}

BOOL HitableCombo::Hit(const Ray &r, float t_min, float t_max, HitRecord &out_rec) const
{
	BOOL hitAnything = FALSE;
	for (UINT32 i = 0; i < m_hitableCount; ++i)
	{
		if (m_hitableList[i]->Hit(r, t_min, t_max, out_rec))
		{
			t_max = out_rec.m_time; // closest so far
			hitAnything = TRUE;
		}
	}
	return hitAnything;
}

void HitableCombo::BindMaterial(IMaterial *m)
{
	for (UINT32 i = 0; i < m_hitableCount; ++i)
	{
		m_hitableList[i]->BindMaterial(m);
	}
}

AABB HitableCombo::BoundingBox() const
{
	assert(m_hitableCount > 0);
	AABB box = m_hitableList[0]->BoundingBox();
	for (UINT32 i = 1; i < m_hitableCount; ++i)
	{
		box = CombineAABB(m_hitableList[i]->BoundingBox(), box);
	}
	return box;
}

BOOL TranslatedInstance::Hit(const Ray &r, float t_min, float t_max, HitRecord &out_rec) const
{
	BOOL hitMe = FALSE;
	Ray moved_r(r.m_org - m_offset, r.m_dir);
	if (m_hitable->Hit(moved_r, t_min, t_max, out_rec))
	{
		out_rec.m_position += m_offset;
		hitMe = TRUE;
	}
	return hitMe;
}

AABB TranslatedInstance::BoundingBox() const
{
	AABB box = m_hitable->BoundingBox();
	box = AABB(box.m_min + m_offset, box.m_max + m_offset);
	return box;
}

RotatedYInstance::RotatedYInstance(IHitable *hitable, float angle)
	: TransformedInstance(hitable)
	, m_sinTheta(sinf(angle))
	, m_cosTheta(cosf(angle))
{
	// caculate new AABB after rotated
	m_boundingBox = m_hitable->BoundingBox();
	Vec3 _min(FLT_MAX, FLT_MAX, FLT_MAX);
	Vec3 _max(-FLT_MAX, -FLT_MAX, -FLT_MAX);

	for (UINT32 i = 0; i < 2; ++i) // 0 or 1
	{
		for (UINT32 j = 0; j < 2; ++j) // 0 or 1
		{
			for (UINT32 k = 0; k < 2; ++k) // 0 or 1
			{
				float x = i * m_boundingBox.m_max.x() + (1 - i) * m_boundingBox.m_min.x();
				float y = i * m_boundingBox.m_max.y() + (1 - i) * m_boundingBox.m_min.y();
				float z = i * m_boundingBox.m_max.z() + (1 - i) * m_boundingBox.m_min.z();
				float newx = m_cosTheta * x + m_sinTheta * z;
				float newz = -m_sinTheta * x + m_cosTheta * z;
				Vec3 tester(newx, y, newz);
				for (UINT32 axis = 0; axis < 3; ++axis)
				{
					if (tester[axis] > _max[axis])
						_max.set(axis, tester[axis]);
					if (tester[axis] < _min[axis])
						_min.set(axis, tester[axis]);
				}
			}
		}
	}
	m_boundingBox = AABB(_min, _max);
}

BOOL RotatedYInstance::Hit(const Ray &r, float t_min, float t_max, HitRecord &out_rec) const
{
	BOOL hitMe = FALSE;
	Vec3 org = r.m_org;
	Vec3 dir = r.m_dir;
	org.set(0, m_cosTheta * r.m_org[0] - m_sinTheta * r.m_org[2]);
	org.set(2, m_sinTheta * r.m_org[0] + m_cosTheta * r.m_org[2]);
	dir.set(0, m_cosTheta * r.m_dir[0] - m_sinTheta * r.m_dir[2]);
	dir.set(2, m_sinTheta * r.m_dir[0] + m_cosTheta * r.m_dir[2]);
	Ray moved_r(org, dir);
	if (m_hitable->Hit(moved_r, t_min, t_max, out_rec))
	{
		Vec3 pos = out_rec.m_position;
		Vec3 nor = out_rec.m_normal;
		pos.set(0, m_cosTheta * out_rec.m_position[0] + m_sinTheta * out_rec.m_position[2]);
		pos.set(2, -m_sinTheta * out_rec.m_position[0] + m_cosTheta * out_rec.m_position[2]);
		nor.set(0, m_cosTheta * out_rec.m_normal[0] + m_sinTheta * out_rec.m_normal[2]);
		nor.set(2, -m_sinTheta * out_rec.m_normal[0] + m_cosTheta * out_rec.m_normal[2]);
		out_rec.m_position = pos;
		out_rec.m_normal = nor;
		hitMe = TRUE;
	}
	return hitMe;
}
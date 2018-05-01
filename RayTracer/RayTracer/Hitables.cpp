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
	// dot(org + t * dir - center, org + t * dir - center ) = radius * radius
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
		if (t < t_max && t > t_min)
		{
			out_rec.m_time = t;
			out_rec.m_position = r.PointAt(t);
			out_rec.m_normal = (out_rec.m_position - m_center) / m_radius; // same as normalize, cos the length is know as m_radius
			return TRUE; // the nearest hitting on ray direction
		}
		t = (-b + sqrt(discriminant)) / (2.0f * a);
		if (t < t_max && t > t_min)
		{
			out_rec.m_time = t;
			out_rec.m_position = r.PointAt(t);
			out_rec.m_normal = (out_rec.m_position - m_center) / m_radius; // same as normalize, cos the length is know as m_radius
			return TRUE; // the farthest hitting on ray direction
		}
	}
	return FALSE; // no hit
}

HitableCombo::HitableCombo(Hitable **pointerArray, UINT32 arraySize)
	: m_pointerArray(pointerArray)
	, m_arraySize(arraySize)
{

}

BOOL HitableCombo::Hit(const Ray &r, float t_min, float t_max, HitRecord &out_rec) const
{
	HitRecord rec;
	BOOL hitAnything = FALSE;
	float cloestSoFar = t_max;
	for (UINT32 i = 0; i < m_arraySize; i++)
	{
		if (m_pointerArray[i]->Hit(r, t_min, cloestSoFar, rec))
		{
			hitAnything = TRUE;
			cloestSoFar = rec.m_time;
			out_rec = rec;
		}
	}
	return hitAnything;
}
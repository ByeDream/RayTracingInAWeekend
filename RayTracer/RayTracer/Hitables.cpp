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
		if (t < t_max && t > t_min)
		{
			out_rec.m_time = t;
			out_rec.m_position = r.PointAt(t);
			out_rec.m_normal = (out_rec.m_position - m_center) / m_radius; // same as normalize, cos the length is know as m_radius
			out_rec.m_hitMaterial = m_material;
			return TRUE; // the nearest hitting on ray direction
		}
		t = (-b + sqrt(discriminant)) / (2.0f * a);
		if (t < t_max && t > t_min)
		{
			out_rec.m_time = t;
			out_rec.m_position = r.PointAt(t);
			out_rec.m_normal = (out_rec.m_position - m_center) / m_radius; // same as normalize, cos the length is know as m_radius
			out_rec.m_hitMaterial = m_material;
			return TRUE; // the farthest hitting on ray direction
		}
	}
	return FALSE; // no hit
}

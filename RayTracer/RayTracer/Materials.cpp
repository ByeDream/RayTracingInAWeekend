#include "stdafx.h"
#include "Materials.h"

#include "Ray.h"
#include "Hitables.h"
#include "RandomFloat.h"

namespace
{
	// helpers:
	Vec3 romdomInUnitSphere()
	{
		Vec3 p;
		do
		{
			p = 2.0f * Vec3(Random(), Random(), Random()) - Vec3(1.0f, 1.0f, 1.0f);
		} while (p.squared_length() >= 1.0f);

		return p;
	}

	Vec3 reflect(const Vec3 &v, const Vec3 &n)
	{
		return v - 2 * dot(v, n) * n;
	}
}

BOOL Lambertian::Scatter(const Ray &r_in, const HitRecord &rec, Vec3 &attenuation, Ray &r_scattered) const
{
	// For simplicity,  scatter always and attenuate by its reflectance R, 
	// Or it can scatter with no attenuation but absorb the fraction 1 - R of the rays
	// Or it could be a mixture of those strategies, like only scatter with some probability p and have attenuation be albedo / p


	// Scatter a ray back to the air with a random direction from the unit radius sphere that is tangent to the hitpoint
	// recursively sample the indirect light with absorb half the energy(50% reflectors), until reach the sky light 
	Vec3 target = rec.m_position + rec.m_normal + romdomInUnitSphere();
	r_scattered = Ray(rec.m_position, target - rec.m_position);
	attenuation = m_albedo;
	return TRUE; // always
}


BOOL Metal::Scatter(const Ray &r_in, const HitRecord &rec, Vec3 &attenuation, Ray &r_scattered) const
{
	Vec3 r_reflected = reflect(normalize(r_in.m_dir), rec.m_normal);
	r_scattered = Ray(rec.m_position, r_reflected + m_fuzziness * romdomInUnitSphere());
	attenuation = m_albedo;
	return (dot(r_scattered.m_dir, rec.m_normal) > 0); // absorb the scatter ray if it is below the surface
}

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

	BOOL reflect(const Vec3 &v, const Vec3 &n, Vec3 &reflected)
	{
		reflected = v - 2 * dot(v, n) * n;
		return TRUE;  //(dot(reflected, n) > 0); // absorb the scatter ray if it is below the surface
	}

	BOOL refract(const Vec3 &v, const Vec3 &n, float ni_over_nt, Vec3 &refracted)
	{
		// Snell's law
		float dt = dot(v, n);
		float discriminant = 1.0f - ni_over_nt * ni_over_nt * (1 - dt * dt);
		BOOL ret = discriminant > 0;
		if (ret) // if false, there is no real solution to Snell's law, called "total internal reflection"
			refracted = ni_over_nt * (v - n * dt) - n * sqrt(discriminant);
		return ret;
	}

	// Schlick's approximation, named after Christophe Schlick, 
	// is a formula for approximating the contribution of the Fresnel factor in the specular reflection of light from a non-conducting interface (surface) between two media.
	float schlick(float cosine, float ref_idx)
	{
		float r0 = (1 - ref_idx) / (1 + ref_idx);
		r0 = r0 * r0;
		return r0 + (1 - r0) * pow((1 - cosine), 5);
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
	Vec3 r_reflected;
	reflect(normalize(r_in.m_dir), rec.m_normal, r_reflected);
	r_scattered = Ray(rec.m_position, r_reflected + m_fuzziness * romdomInUnitSphere());
	attenuation = m_albedo;
	return (dot(r_scattered.m_dir, rec.m_normal) > 0); // absorb the scatter ray if it is below the surface
}

BOOL Dielectric::Scatter(const Ray &r_in, const HitRecord &rec, Vec3 &attenuation, Ray &r_scattered) const
{
	Vec3 outward_normal;
	Vec3 uv = normalize(r_in.m_dir);
	

	float ni_over_nt;
	attenuation = Vec3(1.0f, 1.0f, 1.0f); // Dielectrics absorb nothing
	Vec3 r_refracted;
	float reflect_prob;
	float cosine;
	if (dot(uv, rec.m_normal) > 0) {
		// from internal to outside
		outward_normal = -rec.m_normal;
		ni_over_nt = m_refractiveIndex;  // device by air ref index(1.0f)
		cosine = m_refractiveIndex * dot(uv, rec.m_normal);
	}
	else
	{
		// from outside to internal
		outward_normal = rec.m_normal;
		ni_over_nt = 1.0f / m_refractiveIndex;  // device by air ref index(1.0f)
		cosine = -dot(uv, rec.m_normal);
	}

	if (refract(uv, outward_normal, ni_over_nt, r_refracted))
	{
		reflect_prob = schlick(cosine, m_refractiveIndex);
	}
	else
	{
		// total internal reflection
		reflect_prob = 1.0f;
	}

	if (Random() < reflect_prob) {
		Vec3 r_reflected;
		reflect(uv, rec.m_normal, r_reflected);
		r_scattered = Ray(rec.m_position, r_reflected);
	}
	else
	{
		r_scattered = Ray(rec.m_position, r_refracted);
	}
	return TRUE;
}

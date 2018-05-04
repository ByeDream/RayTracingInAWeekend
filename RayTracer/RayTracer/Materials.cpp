#include "stdafx.h"
#include "Materials.h"

#include "Ray.h"
#include "Hitables.h"
#include "Randomizer.h"
#include "Optics.h"

BOOL Lambertian::Scatter(const Ray &r_in, const HitRecord &rec, Vec3 &attenuation, Ray &r_scattered) const
{
	// For simplicity,  scatter always and attenuate by its reflectance R, 
	// Or it can scatter with no attenuation but absorb the fraction 1 - R of the rays
	// Or it could be a mixture of those strategies, like only scatter with some probability p and have attenuation be albedo / p


	// Scatter a ray back to the air with a random direction from the unit radius sphere that is tangent to the hitpoint
	// recursively sample the indirect light with absorb half the energy(50% reflectors), until reach the sky light 
	Vec3 target = rec.m_position + rec.m_normal + Randomizer::RomdomInUnitSphere();
	r_scattered = Ray(rec.m_position, target - rec.m_position);
	attenuation = m_albedo;
	return TRUE; // always
}


BOOL Metal::Scatter(const Ray &r_in, const HitRecord &rec, Vec3 &attenuation, Ray &r_scattered) const
{
	Vec3 r_reflected;
	Optics::Reflect(normalize(r_in.m_dir), rec.m_normal, r_reflected);
	r_scattered = Ray(rec.m_position, r_reflected + m_fuzziness * Randomizer::RomdomInUnitSphere());
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

	if (Optics::Refract(uv, outward_normal, ni_over_nt, r_refracted))
	{
		reflect_prob = Optics::Schlick(cosine, m_refractiveIndex);
	}
	else
	{
		// total internal reflection
		reflect_prob = 1.0f;
	}

	if (Randomizer::RandomUNorm() < reflect_prob) {
		Vec3 r_reflected;
		Optics::Reflect(uv, rec.m_normal, r_reflected);
		r_scattered = Ray(rec.m_position, r_reflected);
	}
	else
	{
		r_scattered = Ray(rec.m_position, r_refracted);
	}
	return TRUE;
}

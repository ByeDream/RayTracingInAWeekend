#pragma once

#include "Vec3.h"

class Ray;
struct HitRecord;

class Material
{
public:
	virtual BOOL Scatter(const Ray &r_in, const HitRecord &rec, Vec3 &attenuation, Ray &r_scattered) const = 0;
};

class Lambertian : public Material
{
public:
	Lambertian(const Vec3 &albedo) : m_albedo(albedo) {}
	virtual BOOL Scatter(const Ray &r_in, const HitRecord &rec, Vec3 &attenuation, Ray &r_scattered) const override;
	Vec3 m_albedo; // the reflectance
};

class Metal : public Material
{
public:
	Metal(const Vec3 &albedo, float fuzziness) : m_albedo(albedo) { m_fuzziness = (fuzziness < 1.0f) ? ((fuzziness >= 0.0f) ? fuzziness : 0.0f) : 1.0f; }
	virtual BOOL Scatter(const Ray &r_in, const HitRecord &rec, Vec3 &attenuation, Ray &r_scattered) const override;
	Vec3 m_albedo; // the reflectance
	float  m_fuzziness;
};

class Dielectric : public Material
{
public:
	Dielectric(float refractiveIndex) : m_refractiveIndex(refractiveIndex) { }
	virtual BOOL Scatter(const Ray &r_in, const HitRecord &rec, Vec3 &attenuation, Ray &r_scattered) const override;
	//Vec3 m_albedo; // the reflectance
	//float  m_fuzziness;
	float m_refractiveIndex;
};
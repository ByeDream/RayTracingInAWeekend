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

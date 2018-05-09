#pragma once

#include "Vec3.h"

class Ray;
struct HitRecord;

class IMaterial
{
public:
	virtual BOOL Scatter(const Ray &r_in, const HitRecord &rec, Vec3 &attenuation, Ray &r_scattered) const = 0;
	virtual size_t GetDataSize() const = 0;
	virtual const void * GetDataPtr() const = 0;
};

class Lambertian : public IMaterial
{
public:
	struct Data // always use XMFLOAT4 for fix padding
	{
		XMFLOAT4 m_albedo; // the reflectance
	};
	Data m_data;

	Lambertian(const Vec3 &albedo);
	virtual BOOL Scatter(const Ray &r_in, const HitRecord &rec, Vec3 &attenuation, Ray &r_scattered) const override;
	virtual size_t GetDataSize() const override { return sizeof(Lambertian::Data); }
	virtual const void * GetDataPtr() const override { return &m_data; }
};

class Metal : public IMaterial
{
public:
	struct Data // always use XMFLOAT4 for fix padding
	{
		XMFLOAT4 m_albedo; // the reflectance
		XMFLOAT4 m_fuzziness;
	};
	Data m_data;

	Metal(const Vec3 &albedo, float fuzziness);
	
	
	virtual BOOL Scatter(const Ray &r_in, const HitRecord &rec, Vec3 &attenuation, Ray &r_scattered) const override;
	virtual size_t GetDataSize() const override { return sizeof(Metal::Data); }
	virtual const void * GetDataPtr() const override { return &m_data; }
};

class Dielectric : public IMaterial
{
public:
	struct Data // always use XMFLOAT4 for fix padding
	{
		XMFLOAT4 m_refractiveIndex;
	};
	Data m_data;

	Dielectric(float refractiveIndex); 
	virtual BOOL Scatter(const Ray &r_in, const HitRecord &rec, Vec3 &attenuation, Ray &r_scattered) const override;
	virtual size_t GetDataSize() const override { return sizeof(Metal::Data); }
	virtual const void * GetDataPtr() const override { return &m_data; }
};
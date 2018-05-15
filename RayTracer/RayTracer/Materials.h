#pragma once

#include "Vec3.h"

class ITexture2D;

enum MaterialID
{
	MID_LAMBERTIAN = 0,
	MID_METAL,
	MID_DIELECTRIC,

	MID_COUNT
};

class Ray;
struct HitRecord;

class IMaterial
{
public:
	virtual BOOL Scatter(const Ray &r_in, const HitRecord &rec, Vec3 &attenuation, Ray &r_scattered) const = 0;
	virtual size_t GetDataSize() const = 0;
	virtual const void * GetDataPtr() const = 0;
	virtual MaterialID GetID() const = 0;
};

class Lambertian : public IMaterial
{
public:
	struct Data // always use XMFLOAT4 for fix padding
	{
		XMFLOAT4 m_albedo; // the reflectance
	};
	Data m_data;

	ITexture2D *m_diffuse{ nullptr };

	Lambertian(const Vec3 &albedo, ITexture2D *diffuse);
	virtual ~Lambertian();
	virtual BOOL Scatter(const Ray &r_in, const HitRecord &rec, Vec3 &attenuation, Ray &r_scattered) const override;
	virtual size_t GetDataSize() const override { return sizeof(Lambertian::Data); }
	virtual const void * GetDataPtr() const override { return &m_data; }
	virtual MaterialID GetID() const override { return MID_LAMBERTIAN; }
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
	virtual MaterialID GetID() const override { return MID_METAL; }
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
	virtual MaterialID GetID() const override { return MID_DIELECTRIC; }
};
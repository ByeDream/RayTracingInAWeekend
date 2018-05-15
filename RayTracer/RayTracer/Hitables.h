#pragma once

#include "Vec3.h"

class Ray;
class IMaterial;

struct HitRecord
{
	float m_time;
	Vec3 m_position;
	Vec3 m_normal;
	float m_u;
	float m_v;
	IMaterial *m_hitMaterial;
};

// General hitable interface
class IHitable
{
public:
	virtual BOOL Hit(const Ray &r, float t_min, float t_max, HitRecord &out_rec) const = 0;
};

// Sphere hitable
class SphereHitable : public IHitable
{
public:
	Vec3						m_center;
	float						m_radius;
	IMaterial *					m_material{ nullptr };

	SphereHitable() = default;
	virtual ~SphereHitable() = default;
	SphereHitable(const Vec3 &center, float radius);
	virtual BOOL Hit(const Ray &r, float t_min, float t_max, HitRecord &out_rec) const override;
	void BindMaterial(IMaterial *m) { m_material = m; }

private:
	void CalculateUV(HitRecord &rec) const;
};

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
	IMaterial * m_material{ nullptr };

	virtual ~IHitable() = default;
	virtual BOOL Hit(const Ray &r, float t_min, float t_max, HitRecord &out_rec) const = 0;
	void BindMaterial(IMaterial *m) { m_material = m; }
};

// Sphere hitable
class SphereHitable : public IHitable
{
public:
	Vec3						m_center;
	float						m_radius;

	SphereHitable(const Vec3 &center, float radius);
	virtual BOOL Hit(const Ray &r, float t_min, float t_max, HitRecord &out_rec) const override;

private:
	void CalculateUV(HitRecord &rec) const;
};

// Axis-aligned rectangle hitable
class AxisAlignedRectHitable : public IHitable
{
public:
	float m_a0, m_a1, m_b0, m_b1, m_c;
	UINT32 m_aAxisIndex, m_bAxisIndex, m_cAxisIndex;
	AxisAlignedRectHitable(UINT32 aAxisIndex, UINT32 bAxisIndex, float a0, float a1, float b0, float b1, float c);
	virtual BOOL Hit(const Ray &r, float t_min, float t_max, HitRecord &out_rec) const override;
};

// Hitable Combo

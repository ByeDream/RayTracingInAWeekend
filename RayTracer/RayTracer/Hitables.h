#pragma once

#include "Vec3.h"

class Ray;
class Material;

struct HitRecord
{
	float m_time;
	Vec3 m_position;
	Vec3 m_normal;
	Material *m_hitMaterial;
};

// General hitable interface
class Hitable
{
public:
	virtual BOOL Hit(const Ray &r, float t_min, float t_max, HitRecord &out_rec) const = 0;
	void BindMaterial(Material *m) { m_material = m; }
	Material *m_material{ nullptr };
};

// Sphere hitable
class SphereHitable : public Hitable
{
public:
	Vec3						m_center;
	float						m_radius;

	SphereHitable() = default;
	SphereHitable(const Vec3 &center, float radius);
	virtual BOOL Hit(const Ray &r, float t_min, float t_max, HitRecord &out_rec) const override;
};

// Hitable Combo
class HitableCombo: public Hitable
{
public:
	const std::vector<Hitable *> &m_hitableListRef;

	HitableCombo() = default;
	HitableCombo(const std::vector<Hitable *> &hitableListRef) : m_hitableListRef(hitableListRef) {}
	virtual BOOL Hit(const Ray &r, float t_min, float t_max, HitRecord &out_rec) const override;
};
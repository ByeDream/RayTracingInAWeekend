#pragma once

#include "Vec3.h"

class Ray;

struct HitRecord
{
	float m_time;
	Vec3 m_position;
	Vec3 m_normal;
};

// General hitable interface
class Hitable
{
public:
	virtual BOOL Hit(const Ray &r, float t_min, float t_max, HitRecord &out_rec) const = 0;
};

// Sphere hitable
class SphereHitable : public Hitable
{
public:
	Vec3						m_center;
	float						m_radius;

	SphereHitable() = default;
	SphereHitable(const Vec3 &center, float radius);
	virtual BOOL Hit(const Ray &r, float t_min, float t_max, HitRecord &out_rec) const;
};

// Hitable Combo
class HitableCombo: public Hitable
{
public:
	Hitable **					m_pointerArray;
	UINT32						m_arraySize;

	HitableCombo() = default;
	HitableCombo(Hitable **pointerArray, UINT32 arraySize);
	virtual BOOL Hit(const Ray &r, float t_min, float t_max, HitRecord &out_rec) const;
};
#pragma once

#include "Vec3.h"

class Ray;
class IMaterial;
class IMotion;
class SimpleCamera;

struct HitRecord
{
	float m_time;
	Vec3 m_position;
	Vec3 m_normal;
	IMaterial *m_hitMaterial;
};

// General hitable interface
class IHitable
{
public:
	virtual BOOL Hit(const Ray &r, float t_min, float t_max, HitRecord &out_rec) const = 0;
	virtual void OnUpdate(SimpleCamera *camera, float elapsedSeconds) = 0;
};

// Sphere hitable
class SphereHitable : public IHitable
{
public:
	Vec3						m_center;
	float						m_radius;
	IMaterial *					m_material{ nullptr };
	IMotion	*					m_motion{ nullptr };

	SphereHitable() = default;
	virtual ~SphereHitable() = default;
	SphereHitable(const Vec3 &center, float radius);
	virtual BOOL Hit(const Ray &r, float t_min, float t_max, HitRecord &out_rec) const override;
	virtual void OnUpdate(SimpleCamera *camera, float elapsedSeconds) override;
	void BindMaterial(IMaterial *m) { m_material = m; }
	void BindMotion(IMotion *m) { m_motion = m; }
};

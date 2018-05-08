#pragma once

#include "Vec3.h"

class Mesh;
class IHitable;
class IMaterial;

class Object
{
public:
	virtual ~Object() = default;
	Vec3						m_position;
	float						m_scale;
	// TODO for rotation

	Mesh *						m_mesh;
	IHitable *					m_hitable;
	IMaterial *					m_material;
};

class SimpleSphereObject : public Object
{
public:
	SimpleSphereObject(const Vec3 &center, float radius, Mesh *mesh, IMaterial *material);
	virtual ~SimpleSphereObject();
};

// TODO more
#pragma once

#include "Vec3.h"

class SimpleMesh;
class Hitable;
class Material;

class SimpleObject
{
public:
	Vec3						m_position;
	float						m_scale;
	// TODO for rotation

	SimpleMesh *				m_mesh;
	Hitable *					m_hitable;
	Material *					m_material;
};

class SphereSimpleObject : public SimpleObject
{
public:
	SphereSimpleObject(const Vec3 &center, float radius);
};

// TODO more
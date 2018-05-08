#include "stdafx.h"
#include "SimpleObject.h"
#include "Hitables.h"

SimpleSphereObject::SimpleSphereObject(const Vec3 &center, float radius, Mesh *mesh, IMaterial *material)
{
	m_position = center;
	m_scale = radius;
	m_mesh = mesh;
	m_material = material;
	SphereHitable *hitable = new SphereHitable(center, radius);
	hitable->BindMaterial(material);
	m_hitable = hitable;
}

SimpleSphereObject::~SimpleSphereObject()
{
	if (m_hitable)
	{
		delete m_hitable;
		m_hitable = nullptr;
	}
}

#include "stdafx.h"
#include "SimpleTexture2D.h"

Vec3 SimpleTexture2D_Checker::Sample(float u, float v, const Vec3 &p) const
{
	float sines = sinf(10 * p.x()) * sinf(10 * p.y()) * sinf(10 * p.z());
	return sines < 0 ? m_color0 : m_color1;
}

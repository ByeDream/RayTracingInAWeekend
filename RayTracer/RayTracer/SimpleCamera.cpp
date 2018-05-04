#include "stdafx.h"
#include "SimpleCamera.h"

#include "Randomizer.h"

SimpleCamera::SimpleCamera(const Vec3 &lookFrom, const Vec3 &lookAt, const Vec3 &ViewUp, float fov, float aspectRatio, float aperture, float focus_dist)
	: m_origin(lookFrom)
{
	// right hand coordinate
	mLensRadius = aperture / 2.0f;

	m_w = normalize(lookFrom - lookAt);
	m_u = normalize(cross(ViewUp, m_w));
	m_v = cross(m_w, m_u);
	const float theta = fov * (float)M_PI / 180.0f;
	const float viewHalfHeight = tan(theta / 2.0f) * focus_dist; // * minZ
	const float viewHalfWidth = viewHalfHeight * aspectRatio;

	m_viewTopLeftCorner = m_origin - viewHalfWidth * m_u + viewHalfHeight * m_v - focus_dist * m_w;
	m_viewHorizontal = 2.0f * viewHalfWidth * m_u;
	m_viewVertical = -2.0f * viewHalfHeight * m_v;
}

Ray SimpleCamera::GetRay(float u, float v) const
{
	Vec3 rd = mLensRadius * Randomizer::RandomInUnitDisk();
	Vec3 offset = m_u * rd.x() + m_v * rd.y();
	return Ray(m_origin + offset, m_viewTopLeftCorner + u * m_viewHorizontal + v * m_viewVertical - m_origin - offset);
}
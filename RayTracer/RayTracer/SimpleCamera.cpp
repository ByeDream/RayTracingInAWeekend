#include "stdafx.h"
#include "SimpleCamera.h"

SimpleCamera::SimpleCamera(const Vec3 &pos, const Vec3 &up, const Vec3 &forward, float aspectRatio, float minZ, float maxZ)
	: m_origin(pos)
{
	// right hand coordinate
	// ignore the camera direction at the moment, it always get the up alones +y-axis(world), and looks to -z-axis(world)
	// alse ignore FOV, maxZ at the moment

	const float viewPlaneHalfHeight = 1.0f;
	const float viewPlaneHalfWidth = viewPlaneHalfHeight * aspectRatio;

	m_viewPlaneHigherLeftCorner = m_origin + Vec3(-viewPlaneHalfWidth, viewPlaneHalfHeight, -minZ);
	m_viewPlaneHorizontal = Vec3(2.0f * viewPlaneHalfWidth, 0.0f, 0.0f);
	m_viewPlaneVertical = Vec3(0.0f, -2.0f * viewPlaneHalfHeight, 0.0f);
}

Ray SimpleCamera::GetRay(float u, float v) const
{
	return Ray(m_origin, m_viewPlaneHigherLeftCorner + u * m_viewPlaneHorizontal + v * m_viewPlaneVertical);
}
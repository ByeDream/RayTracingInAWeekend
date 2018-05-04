#include "stdafx.h"
#include "SimpleCamera.h"

#include "Randomizer.h"
#include "World.h"
SimpleCamera::SimpleCamera(
	const Vec3 &lookFrom,
	const Vec3 &lookAt,
	const Vec3 &viewUp,
	float fov,
	float aspectRatio,
	float minZ,
	float maxZ,
	float aperture,
	const World *world)
	: m_initialOrigin(lookFrom)
	, m_initialFocus(lookAt)
	, m_vup(viewUp)
	, m_fov(fov)
	, m_aspectRatio(aspectRatio)
	, m_nearPlane(minZ)
	, m_farPlane(maxZ)
	, m_lensRadius(aperture / 2.0f)
	, m_moveSpeed(0.0f)
	, m_turnSpeed(0.0f)
	, m_world(world)
{
	Reset();
}

Ray SimpleCamera::GetRay(float u, float v) const
{
	Vec3 rd = m_lensRadius * Randomizer::RandomInUnitDisk();
	Vec3 offset = m_u * rd.x() + m_v * rd.y();
	return Ray(m_origin + offset, m_viewTopLeftCorner + u * m_viewHorizontal + v * m_viewVertical - m_origin - offset);
}

void SimpleCamera::OnUpdate(float elapsedSeconds)
{

}

void SimpleCamera::Reset()
{
	m_origin = m_initialOrigin;
	m_focus = m_initialFocus;
	InternalUpdate();
}


void SimpleCamera::InternalUpdate()
{

	// redo auto focus here as lone as we have update, to be opti later
	AutoFocus();
	// right hand coordinate

	// TODO using ray tracing to update it automatically
	float focusDist = (m_origin - m_focus).length();


	m_w = normalize(m_origin - m_focus);
	m_u = normalize(cross(m_vup, m_w));
	m_v = cross(m_w, m_u);
	const float theta = m_fov * (float)M_PI / 180.0f;
	const float viewHalfHeight = tan(theta / 2.0f) * focusDist; // * minZ
	const float viewHalfWidth = viewHalfHeight * m_aspectRatio;

	m_viewTopLeftCorner = m_origin - viewHalfWidth * m_u + viewHalfHeight * m_v - focusDist * m_w;
	m_viewHorizontal = 2.0f * viewHalfWidth * m_u;
	m_viewVertical = -2.0f * viewHalfHeight * m_v;
}

DirectX::XMMATRIX SimpleCamera::GetViewMatrix() const
{

}
DirectX::XMMATRIX SimpleCamera::GetProjectionMatrix() const
{

}

BOOL SimpleCamera::AutoFocus()
{
	return TRUE;
}
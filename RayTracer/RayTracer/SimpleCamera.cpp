#include "stdafx.h"
#include "SimpleCamera.h"

#include "Randomizer.h"
#include "World.h"
#include "Hitables.h"
#include "Ray.h"

#include "InputListener.h"

using namespace std;

SimpleCamera::SimpleCamera(
	const Vec3 &lookFrom,
	const Vec3 &lookAt,
	const Vec3 &viewUp,
	float fov,
	float aspectRatio,
	float minZ,
	float maxZ,
	float aperture,
	const World *world,
	InputListener *inputListener
)
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
	, m_inputListener(inputListener)
{
	m_inputListener->RegisterKey('W');
	m_inputListener->RegisterKey('A');
	m_inputListener->RegisterKey('S');
	m_inputListener->RegisterKey('D');
	m_inputListener->RegisterKey(VK_LEFT);
	m_inputListener->RegisterKey(VK_UP);
	m_inputListener->RegisterKey(VK_RIGHT);
	m_inputListener->RegisterKey(VK_DOWN);
	m_inputListener->RegisterKey(VK_ESCAPE);

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

	

	// Calculate the move vector in camera space.
	Vec3 move(0.0f, 0.0f, 0.0f);
	if (m_inputListener->WhenHoldKey('A'))
		move += Vec3(-1.0f, 0.0f, 0.0f);
	if (m_inputListener->WhenHoldKey('D'))
		move += Vec3(1.0f, 0.0f, 0.0f);
	if (m_inputListener->WhenHoldKey('W'))
		move += Vec3(0.0f, 0.0f, -1.0f);
	if (m_inputListener->WhenHoldKey('S'))
		move += Vec3(0.0f, 0.0f, 1.0f);
	move.normalize();


	Vec3 offset = m_u * move.x() + m_w * move.z();
	m_origin += offset;


	// Calculate the rotatle
	float yawAngle = 0.0f;
	float pitchAngle = 0.0f;
	if (m_inputListener->WhenHoldKey(VK_LEFT))
		yawAngle += 0.3f;
	if (m_inputListener->WhenHoldKey(VK_RIGHT))
		yawAngle -= 0.3f;
	if (m_inputListener->WhenHoldKey(VK_UP))
		pitchAngle += 0.3f;
	if (m_inputListener->WhenHoldKey(VK_DOWN))
		pitchAngle -= 0.3f;
	cout << yawAngle << endl;
	cout << pitchAngle << endl;

	DirectX::XMMATRIX rotatle = DirectX::XMMatrixRotationRollPitchYaw(pitchAngle, yawAngle, 0.0f);
	m_w = DirectX::XMVector3Transform(m_w.m_simd, rotatle);

	m_focus = m_origin - m_w;



	// redo auto focus here as lone as we have update, to be opti later
	AutoFocus();
	InternalUpdate();
}

void SimpleCamera::HelpInfo()
{

}

void SimpleCamera::Reset()
{
	cout << "[SimpleCamera] Reset" << endl;
	m_origin = m_initialOrigin;
	m_focus = m_initialFocus;
	InternalUpdate();
}


void SimpleCamera::InternalUpdate()
{
	// right hand coordinate
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
	return DirectX::XMMATRIX();
}
DirectX::XMMATRIX SimpleCamera::GetProjectionMatrix() const
{
	return DirectX::XMMATRIX();
}

BOOL SimpleCamera::AutoFocus()
{
	HitRecord rec;
	Ray r(m_origin, -m_w);
	if (m_world->GetRootHitable()->Hit(r, 0.01f, FLT_MAX, rec))
	{
		m_focus = rec.m_position;
	}

	return TRUE;
}
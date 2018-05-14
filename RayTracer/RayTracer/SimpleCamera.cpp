#include "stdafx.h"
#include "SimpleCamera.h"

#include "Randomizer.h"
#include "World.h"
#include "Hitables.h"
#include "Ray.h"

#include "InputListener.h"

using namespace std;

SimpleCamera::SimpleCamera(const Vec3 &lookFrom, const Vec3 &lookAt, float fov, float aspectRatio, float minZ, float maxZ, float aperture, float shutter, const World *world, InputListener *inputListener)
	: m_initialOrigin(lookFrom)
	, m_initialFocus(lookAt)
	, m_vup(0.0f, 1.0f, 0.0f) // no roll
	, m_fov(fov)
	, m_aspectRatio(aspectRatio)
	, m_nearPlane(minZ)
	, m_farPlane(maxZ)
	, m_lensRadius(aperture / 2.0f)
	, m_world(world)
	, m_inputListener(inputListener)
	, m_moveSpeed(0.0f)
	, m_turnSpeed(0.0f)
	, m_emittingDurationInMs(shutter)
{
	m_inputListener->RegisterKey('H');
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
	float emittedTimeInMs = Randomizer::RandomUNorm() * m_emittingDurationInMs;
	Vec3 rd = m_lensRadius * Randomizer::RandomInUnitDisk();
	Vec3 offset = m_u * rd.x() + m_v * rd.y();
	return Ray(m_origin + offset, m_viewTopLeftCorner + u * m_viewHorizontal + v * m_viewVertical - m_origin - offset, emittedTimeInMs);
}

void SimpleCamera::OnUpdate(float elapsedSeconds)
{
	if (m_inputListener->WhenReleaseKey(VK_ESCAPE))
	{
		Reset();
		return;
	}

	// Calculate the move vector in camera space.
	BOOL holdA = m_inputListener->WhenHoldKey('A');
	BOOL holdD = m_inputListener->WhenHoldKey('D');
	BOOL holdW = m_inputListener->WhenHoldKey('W');
	BOOL holdS = m_inputListener->WhenHoldKey('S');
	BOOL holdLeft = m_inputListener->WhenHoldKey(VK_LEFT);
	BOOL holdRight = m_inputListener->WhenHoldKey(VK_RIGHT);
	BOOL holdUp = m_inputListener->WhenHoldKey(VK_UP);
	BOOL holdDown = m_inputListener->WhenHoldKey(VK_DOWN);
	BOOL holdAnyKey = holdA || holdD || holdW || holdS || holdLeft || holdRight || holdUp || holdDown;


	Vec3 move(0.0f, 0.0f, 0.0f);
	if (holdA)
		move += Vec3(-1.0f, 0.0f, 0.0f);
	if (holdD)
		move += Vec3(1.0f, 0.0f, 0.0f);
	if (holdW)
		move += Vec3(0.0f, 0.0f, -1.0f);
	if (holdS)
		move += Vec3(0.0f, 0.0f, 1.0f);

	// Calculate the rotatle
	if (holdLeft)
		m_yaw += m_turnSpeed * elapsedSeconds;
	if (holdRight)
		m_yaw -= m_turnSpeed * elapsedSeconds;
	if (holdUp)
		m_pitch -= m_turnSpeed * elapsedSeconds;
	if (holdDown)
		m_pitch += m_turnSpeed * elapsedSeconds;

	if (holdAnyKey)
	{
		// update origin
		move.normalize();
		move *= m_moveSpeed * elapsedSeconds;
		Vec3 offset = m_u * move.x() + m_w * move.z();
		m_origin += offset;

		// update focus
		float r = cosf(m_pitch);
		Vec3 lookDir(r * sinf(m_yaw), -sinf(m_pitch), r * cosf(m_yaw));
		m_focus = m_origin + lookDir;

		AutoFocus(lookDir);
		InternalUpdate();
	}

	if (m_inputListener->WhenReleaseKey('H'))
	{
		HelpInfo();
	}
}

void SimpleCamera::HelpInfo()
{
	cout << "================SimpleCamera==============" << endl;
	cout << "[Hot keys]" << endl;
	cout << "  [h] Display this message." << endl;
	cout << "  [a,d,w,s] move the camera's position." << endl;
	cout << "  [up, down, left, right] rotate the camera's pitch and yaw." << endl;
	cout << "  [esc] reset the camera" << endl;
	cout << "[Orientation] " << m_origin << " > " << m_focus <<endl;
	cout << "[fov] " << m_fov << endl;
	cout << "[aperture] " << m_lensRadius * 2.0f << endl;
	cout << "[shutter] " << m_emittingDurationInMs << endl;
	cout << "==========================================" << endl;
}

void SimpleCamera::Reset()
{
	cout << "[SimpleCamera] Reset" << endl;
	m_origin = m_initialOrigin;
	m_focus = m_initialFocus;
	Vec3 lookDir = m_focus - m_origin;
	float length = lookDir.length();
	if (length != 0.0f)
	{
		m_pitch = -asinf((lookDir.y() / length));
		if (lookDir.z() > 0.0f)
		{
			m_yaw = atanf((lookDir.x() / lookDir.z()));
		}
		else if (lookDir.z() < 0.0f)
		{
			m_yaw = atanf((lookDir.x() / lookDir.z())) + (float)M_PI;
		}
		else if (lookDir.x() > 0.0f)
		{
			m_yaw = (float)M_PI_2;
		}
		else if (lookDir.x() < 0.0f)
		{
			m_yaw = -(float)M_PI_2;
		}
	}
	else
	{
		m_focus = m_origin + Vec3(0.0f, 0.0f, -1.0f);
		m_pitch = 0.0f;
		m_yaw = (float)M_PI;
	}
	
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

XMMATRIX SimpleCamera::GetViewMatrix() const
{
	return DirectX::XMMatrixLookAtRH(m_origin.m_simd, m_focus.m_simd, m_v.m_simd);
}
XMMATRIX SimpleCamera::GetProjectionMatrix() const
{
	return DirectX::XMMatrixPerspectiveFovRH(m_fov * (float)M_PI / 180.0f, m_aspectRatio, m_nearPlane, m_farPlane);
}

BOOL SimpleCamera::AutoFocus(const Vec3 &lookDir)
{
	HitRecord rec;
	Ray r(m_origin, lookDir, 0.0f);
	if (m_world->Hit(r, 0.01f, FLT_MAX, rec))
	{
		m_focus = rec.m_position;
	}

	return TRUE;
}
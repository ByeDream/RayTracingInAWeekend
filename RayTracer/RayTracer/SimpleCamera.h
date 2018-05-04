#pragma once

#include "Ray.h"

class SimpleCamera
{
public:
	SimpleCamera(
		const Vec3 &lookFrom,
		const Vec3 &lookAt,
		const Vec3 &ViewUp,
		float fov,
		float aspectRatio,
		float minZ,
		float maxZ,
		float aperture, 
		float focus_dist);

	void							SetMoveSpeed(float unitsPerSecond);
	void							SetTurnSpeed(float radiansPerSecond);

	Ray								GetRay(float u, float v) const;
	void							OnUpdate(float elapsedSeconds);

	DirectX::XMMATRIX				GetViewMatrix() const;
	DirectX::XMMATRIX				GetProjectionMatrix() const;

private:
	void							Reset();
	Vec3							m_origin;
	Vec3							m_initialOrigin;
	Vec3							m_focus;
	Vec3							m_initialFocus;

	float							m_fov;
	float							m_aspectRatio;
	float							m_nearPlane;
	float							m_farPlane;

	float							m_moveSpeed;			// Speed at which the camera moves, in units per second.
	float							m_turnSpeed;			// Speed at which the camera turns, in radians per second.


	Vec3 m_viewTopLeftCorner;
	Vec3 m_viewHorizontal;
	Vec3 m_viewVertical;

	
	Vec3 m_u;
	Vec3 m_v;
	Vec3 m_w;
	float mLensRadius;
	//Vec3 m_lookAt;
	//Vec3 m_vup;
};
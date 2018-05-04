#pragma once

#include "Ray.h"

class World;
class InputListener;

class SimpleCamera
{
public:
	SimpleCamera(
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
	);

	inline void						SetMoveSpeed(float unitsPerSecond) { m_moveSpeed = unitsPerSecond; }
	inline void						SetTurnSpeed(float radiansPerSecond) { m_turnSpeed = radiansPerSecond; }

	Ray								GetRay(float u, float v) const;
	void							OnUpdate(float elapsedSeconds);

	DirectX::XMMATRIX				GetViewMatrix() const;
	DirectX::XMMATRIX				GetProjectionMatrix() const;

	void							HelpInfo();

private:
	void							Reset();
	void							InternalUpdate();
	BOOL							AutoFocus();

	Vec3							m_origin;
	Vec3							m_initialOrigin;
	Vec3							m_focus;
	Vec3							m_initialFocus;

	Vec3							m_vup;
	float							m_fov;
	float							m_aspectRatio;
	float							m_nearPlane;
	float							m_farPlane;
	float							m_lensRadius;

	float							m_moveSpeed;			// Speed at which the camera moves, in units per second.
	float							m_turnSpeed;			// Speed at which the camera turns, in radians per second.


	Vec3							m_viewTopLeftCorner;
	Vec3							m_viewHorizontal;
	Vec3							m_viewVertical;
	Vec3							m_u;
	Vec3							m_v;
	Vec3							m_w;

	const World *					m_world{ nullptr };
	InputListener *					m_inputListener{ nullptr };
};
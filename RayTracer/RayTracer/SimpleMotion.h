#pragma once

#include "Vec3.h"

class IMotion
{
public:
	virtual ~IMotion() = default;
	virtual Vec3 Move(const Vec3 &position, float elapsedSeconds) = 0;
};

class SimpleMotionPingpong : public IMotion
{
public:
	SimpleMotionPingpong(const Vec3 &initDir, float acc, float m_minSpeed, float maxSpeed);
	virtual Vec3 Move(const Vec3 &position, float elapsedSeconds) override;

	Vec3 m_direction;
	float m_acceleration;
	float m_speed;
	float m_maxSpeed;
	float m_minSpeed;
};
#include "stdafx.h"
#include "SimpleMotion.h"

SimpleMotionPingpong::SimpleMotionPingpong(const Vec3 &initDir, float acc, float m_minSpeed,float maxSpeed)
	: m_acceleration(acc)
	, m_maxSpeed(maxSpeed)
	, m_speed(m_minSpeed)
{
	m_direction = normalize(initDir);
}

Vec3 SimpleMotionPingpong::Move(const Vec3 &position, float elapsedSeconds)
{
	float offset = m_speed * elapsedSeconds + 0.5f * m_acceleration * elapsedSeconds * elapsedSeconds;
	m_speed += m_acceleration * elapsedSeconds;
	Vec3 newPos = position + m_direction * offset;
	if (m_speed >= m_maxSpeed)
	{
		m_speed = m_minSpeed;
		m_direction = -m_direction;
	}
	return newPos;
}

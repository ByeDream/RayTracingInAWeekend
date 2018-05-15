#pragma once

#include "Vec3.h"

class ITexture2D
{
public:
	virtual Vec3 Sample(float u, float v, const Vec3 &p) const = 0;
};

class SimpleTexture2D_SingleColor : public ITexture2D
{
public:
	SimpleTexture2D_SingleColor(const Vec3 &col) : m_color(col) {}
	virtual Vec3 Sample(float u, float v, const Vec3 &p) const override { return m_color; }
	Vec3 m_color;
};

class SimpleTexture2D_Checker : public ITexture2D
{
public:
	SimpleTexture2D_Checker(const Vec3 &col0, const Vec3 &col1) : m_color0(col0), m_color1(col1) {}
	virtual Vec3 Sample(float u, float v, const Vec3 &p) const override;
	Vec3 m_color0;
	Vec3 m_color1;
};

// just for debug:
class SimpleTexture2D_DisplayUV : public ITexture2D
{
public:
	virtual Vec3 Sample(float u, float v, const Vec3 &p) const override { return Vec3(u, v, 0.0f); }
};
#pragma once

#include "Vec3.h"

class OutputImage;
class Hitable;
class Ray;

class HomemadeRayTracer
{
public:
	HomemadeRayTracer();
	~HomemadeRayTracer();

	void						Trace(OutputImage *image);

private:
	Vec3						Sample(const Ray &r, const Hitable &world) const;
	void						ConstructHitableWorld();
	void						DeconstructHitableWorld();

	Hitable *					m_hitableWorld{ nullptr };
};

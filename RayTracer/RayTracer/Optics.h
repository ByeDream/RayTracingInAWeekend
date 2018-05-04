#pragma once

#include "Vec3.h"

class Optics
{
public:
	static inline BOOL Reflect(const Vec3 &v, const Vec3 &n, Vec3 &reflected)
	{
		reflected = v - 2 * dot(v, n) * n;
		return TRUE;  //(dot(reflected, n) > 0); // absorb the scatter ray if it is below the surface
	}

	static inline BOOL Refract(const Vec3 &v, const Vec3 &n, float ni_over_nt, Vec3 &refracted)
	{
		// Snell's law
		float dt = dot(v, n);
		float discriminant = 1.0f - ni_over_nt * ni_over_nt * (1 - dt * dt);
		BOOL ret = discriminant > 0;
		if (ret) // if false, there is no real solution to Snell's law, called "total internal reflection"
			refracted = ni_over_nt * (v - n * dt) - n * sqrt(discriminant);
		return ret;
	}

	// Schlick's approximation, named after Christophe Schlick, 
	// is a formula for approximating the contribution of the Fresnel factor in the specular reflection of light from a non-conducting interface (surface) between two media.
	static inline float Schlick(float cosine, float ref_idx)
	{
		float r0 = (1 - ref_idx) / (1 + ref_idx);
		r0 = r0 * r0;
		return r0 + (1 - r0) * pow((1 - cosine), 5);
	}
};
#pragma once

#include "Vec3.h"

class Randomizer
{
public:
	// [min, max)
	static inline float RandomMinMax(float min, float max)
	{
		std::random_device rd;
		UINT64 _max = (rd.max)();
		_max += 1;
		double r = static_cast<double>(rd()) / static_cast<double>(_max); // [0.0f, 1.0f)
		r = r * (max - min) + min;
		return static_cast<float>(r);
	}

	// [min, max]
	static inline float RandomMinMax2(float min, float max)
	{
		std::random_device rd;
		double r = static_cast<double>(rd()) / static_cast<double>((rd.max)()); // [0.0f, 1.0f]
		r = r * (max - min) + min;
		return static_cast<float>(r);
	}

	// [0.0f, 1.0f)
	static inline float RandomUNorm()
	{
		return RandomMinMax(0.0f, 1.0f);
	}

	// [0.0f, 1.0f]
	static inline float RandomUNorm2()
	{
		return RandomMinMax2(0.0f, 1.0f);
	}

	static inline Vec3 RomdomInUnitSphere()
	{
		Vec3 p;
		do
		{
			p = Vec3(RandomMinMax2(-1.0f, 1.0f), RandomMinMax2(-1.0f, 1.0f), RandomMinMax2(-1.0f, 1.0f));
		} while (p.squared_length() >= 1.0f);
		return p;
	}

	static inline Vec3 RandomInUnitDisk()
	{
		Vec3 p;
		do
		{
			p = Vec3(RandomMinMax2(-1.0f, 1.0f), RandomMinMax2(-1.0f, 1.0f), 0.0f);
		} while (p.squared_length() >= 1.0f);
		return p;
	}
};

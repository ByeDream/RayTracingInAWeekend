#pragma once

// [min, max)
inline float Random(float min, float max)
{
	std::random_device rd;
	UINT64 _max = (rd.max)();
	_max += 1;
	double r = static_cast<double>(rd()) / static_cast<double>(_max); // [0.0f, 1.0f)
	r = r * (max - min) + min;
	return static_cast<float>(r);
}

// [0.0f, 1.0f)
inline float Random()
{
	return Random(0.0f, 1.0f);
}
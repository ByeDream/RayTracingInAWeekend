#pragma once

// a wrapper class of SIMD vector
// three coordinates suffices
struct Vec3
{
	XMVECTOR m_simd;

	Vec3() = default;
	Vec3(float e0, float e1, float e2) { m_simd = DirectX::XMVectorSet(e0, e1, e2, 0.f); }
	Vec3(XMVECTOR simd) : m_simd(simd) {}

	inline const float x() const { return DirectX::XMVectorGetX(m_simd); }
	inline const float y() const { return DirectX::XMVectorGetY(m_simd); }
	inline const float z() const { return DirectX::XMVectorGetZ(m_simd); }
	inline const float r() const { return DirectX::XMVectorGetX(m_simd); }
	inline const float g() const { return DirectX::XMVectorGetY(m_simd); }
	inline const float b() const { return DirectX::XMVectorGetZ(m_simd); }

	inline const Vec3& operator+() const { return *this; }
	inline const Vec3 operator-() const { return DirectX::XMVectorNegate(m_simd); }
	inline const float operator[](size_t index) const { return DirectX::XMVectorGetByIndex(m_simd, index); }
	inline void set(size_t index, float f) { DirectX::XMVectorSetByIndex(m_simd, f, index); }
	
	inline Vec3& operator+=(const Vec3 &v2) { m_simd = DirectX::XMVectorAdd(m_simd, v2.m_simd); return *this; }
	inline Vec3& operator-=(const Vec3 &v2) { m_simd = DirectX::XMVectorSubtract(m_simd, v2.m_simd); return *this; }
	inline Vec3& operator*=(const Vec3 &v2) { m_simd = DirectX::XMVectorMultiply(m_simd, v2.m_simd); return *this; }
	inline Vec3& operator/=(const Vec3 &v2) { m_simd = DirectX::XMVectorDivide(m_simd, v2.m_simd); return *this; }
	inline Vec3& operator*=(const float f) { m_simd = DirectX::XMVectorScale(m_simd, f); return *this; }
	inline Vec3& operator/=(const float f) { m_simd = DirectX::XMVectorScale(m_simd, 1.f / f); return *this; }

	inline const float length() const { return DirectX::XMVectorGetX(DirectX::XMVector3Length(m_simd)); }
	inline const float squared_length() const { return DirectX::XMVectorGetX(DirectX::XMVector3LengthSq(m_simd)); }
	inline void normalize() { m_simd = DirectX::XMVector3Normalize(m_simd); }
	inline void zero() { m_simd = DirectX::XMVectorZero(); }
};

inline std::istream& operator>>(std::istream &is, Vec3 &v)
{
	XMFLOAT3 float3;
	is >> float3.x >> float3.y >> float3.z;
	v.m_simd = DirectX::XMLoadFloat3(&float3);
	return is;
}

inline std::ostream& operator<<(std::ostream &os, const Vec3 &v)
{
	XMFLOAT3 float3;
	DirectX::XMStoreFloat3(&float3, v.m_simd);
	os << "[" << float3.x << "," << float3.y << "," << float3.z << "]";
	return os;
}

inline Vec3 operator+(const Vec3 &v1, const Vec3 &v2)
{
	return DirectX::XMVectorAdd(v1.m_simd, v2.m_simd);
}

inline Vec3 operator-(const Vec3 &v1, const Vec3 &v2)
{
	return DirectX::XMVectorSubtract(v1.m_simd, v2.m_simd);
}

inline Vec3 operator*(const Vec3 &v1, const Vec3 &v2)
{
	return DirectX::XMVectorMultiply(v1.m_simd, v2.m_simd);
}

inline Vec3 operator/(const Vec3 &v1, const Vec3 &v2)
{
	return DirectX::XMVectorDivide(v1.m_simd, v2.m_simd);
}

inline Vec3 operator*(float f, const Vec3 &v)
{
	return DirectX::XMVectorScale(v.m_simd, f);
}

inline Vec3 operator*(const Vec3 &v, float f)
{
	return DirectX::XMVectorScale(v.m_simd, f);
}

inline Vec3 operator/(const Vec3 &v, float f)
{
	return DirectX::XMVectorScale(v.m_simd, 1.f / f);
}

inline float dot(const Vec3 &v1, const Vec3 &v2)
{
	return DirectX::XMVectorGetX(DirectX::XMVector3Dot(v1.m_simd, v2.m_simd));
}

inline Vec3 cross(const Vec3 &v1, const Vec3 &v2)
{
	return DirectX::XMVector3Cross(v1.m_simd, v2.m_simd);
}

inline Vec3 normalize(const Vec3 &v)
{
	return DirectX::XMVector3Normalize(v.m_simd);
}

inline Vec3 zero()
{
	return DirectX::XMVectorZero();
}
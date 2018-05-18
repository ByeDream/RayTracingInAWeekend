#pragma once

#include "Vec3.h"

class Object;
class SimpleCamera;
class World;
class D3D12Viewer;

class LightSources
{
public:
	LightSources(World *world, std::vector<Object *> &objects, const Vec3 &ambientLight);
	~LightSources();

	void									Update(SimpleCamera *camera, float elapsedSeconds);
	void									ApplyCBV(D3D12Viewer *viewer) const;
	void									BuildD3DRes(D3D12Viewer *viewer, CD3DX12_CPU_DESCRIPTOR_HANDLE &cbvCPUHandle, CD3DX12_GPU_DESCRIPTOR_HANDLE &cbvGPUHandle);

	inline UINT32							GetLightSourceCount() const { return m_lightSourceCountX; }
	inline const Vec3 &						GetAmbientLight() const { return m_ambientLight; }
	inline D3D12_GPU_DESCRIPTOR_HANDLE		GetIllumCbvHandle(UINT32 index) { return m_IllumCbvHandles[index]; }

private:
	World *									m_world{ nullptr };
	std::vector<Object *>					m_lightSources;
	UINT32									m_lightSourceCount{ 0 };
	UINT32									m_lightSourceCountX{ 0 };

	UINT8 *									m_pIllumGlobalConstants{ nullptr };
	ComPtr<ID3D12Resource>					m_illumGlobalConstantBuffer;
	UINT32									m_illumGlobalConstantBufferSize{ 0 };

	UINT8 *									m_pLightSourceConstants{ nullptr };
	ComPtr<ID3D12Resource>					m_lightSourceConstantBuffer;
	UINT32									m_lightSourceConstantBufferSize{ 0 };

	CD3DX12_GPU_DESCRIPTOR_HANDLE *			m_IllumCbvHandles{ nullptr };

	Vec3									m_ambientLight;
};
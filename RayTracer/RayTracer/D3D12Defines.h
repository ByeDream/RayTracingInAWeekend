#pragma once

struct PipelineState
{
	ComPtr<ID3D12RootSignature>					m_RS;
	ComPtr<ID3D12PipelineState>					m_PSO;
};

struct IllumGlobalConstants
{
	XMFLOAT4	ambientIntensity;
	XMFLOAT4	lightSourceCount;
};

struct LightSourceConstants
{
	XMFLOAT4	lightPositionView;
	XMFLOAT4	lightIntensity;
	XMFLOAT4    lightAttenuation;
};

#define MAX_POINT_LIGHT 4
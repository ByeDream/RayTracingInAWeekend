#pragma once

struct PipelineState
{
	ComPtr<ID3D12RootSignature>					m_RS;
	ComPtr<ID3D12PipelineState>					m_PSO;
};

#define MAX_POINT_LIGHT 4
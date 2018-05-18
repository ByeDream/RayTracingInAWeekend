#include "stdafx.h"
#include "LightSources.h"
#include "SimpleObject.h"
#include "Materials.h"
#include "World.h"
#include "SimpleCamera.h"
#include "D3D12Viewer.h"
#include "D3D12Helper.h"
#include "std_cbuffer.h"

#define AMBIENT_INTENSITY_ATTENUATION 0.8f // to simulate global attenuation of indirect light
#define LIGHT_INTENSITY_SCALE 0.5f // as we use diffierent unit definition

LightSources::LightSources(World *world, std::vector<Object *> &objects, const Vec3 &ambientLight)
	: m_world(world)
	, m_ambientLight(ambientLight)
{
	// count light sources, TODO for other kinds of light
	for (auto i = objects.begin(); i != objects.end(); ++i)
	{
		IMaterial *mtl = (*i)->m_material;
		if (mtl && mtl->GetID() == MID_DIFFUSE_LIGHT)
		{
			m_lightSources.push_back(*i);
		}
	}

	m_lightSourceCount = (UINT32)m_lightSources.size();
	m_lightSourceCountX = m_lightSourceCount == 0 ? 1 : m_lightSourceCount; // for constant we atlease keep 1 to matching with shader sig.
}

LightSources::~LightSources()
{
	if (m_IllumCbvHandles)
	{
		delete[] m_IllumCbvHandles;
		m_IllumCbvHandles = nullptr;
	}
}

void LightSources::Update(SimpleCamera *camera, float elapsedSeconds)
{
	IllumGlobalConstants globalConstants;
	DirectX::XMStoreFloat4(&globalConstants.ambientIntensity, (m_ambientLight * AMBIENT_INTENSITY_ATTENUATION).m_simd);
	globalConstants.lightSourceCount.x = (float)m_lightSourceCount;
	memcpy(m_pIllumGlobalConstants + m_illumGlobalConstantBufferSize * m_world->GetFrameIndex(), &globalConstants, sizeof(IllumGlobalConstants));

	LightSourceConstants lightConstants;
	for (UINT32 i = 0; i < m_lightSourceCount; ++i)
	{
		const Object *lightSource = m_lightSources[i];
		const DiffuseLight *lightMtl = reinterpret_cast<const DiffuseLight *>(lightSource->m_material);

		XMVECTOR lightPosWorld = DirectX::XMVectorSet(lightSource->m_position.x(), lightSource->m_position.y(), lightSource->m_position.z(), 1.0f);
		XMVECTOR lightPosView = DirectX::XMVector4Transform(lightPosWorld, camera->GetViewMatrix());

		DirectX::XMStoreFloat4(&lightConstants.lightPositionView, lightPosView);
		lightConstants.lightIntensity = XMFLOAT4(lightMtl->m_data.intensity.x * LIGHT_INTENSITY_SCALE, lightMtl->m_data.intensity.y * LIGHT_INTENSITY_SCALE, lightMtl->m_data.intensity.z * LIGHT_INTENSITY_SCALE, 0.0f);
		lightConstants.lightAttenuation = XMFLOAT4(0.0f, 0.0f, 1.0f, 0.0f); // TBD
		UINT32 index = m_world->GetFrameIndex() * m_lightSourceCount + i;
		memcpy(m_pLightSourceConstants + m_lightSourceConstantBufferSize * index, &lightConstants, sizeof(LightSourceConstants));
	}
}

void LightSources::ApplyCBV(D3D12Viewer *viewer) const
{
	ID3D12GraphicsCommandList *commandList = viewer->GetGraphicsCommandList();
	commandList->SetGraphicsRootDescriptorTable(1, m_IllumCbvHandles[m_world->GetFrameIndex()]);
}

void LightSources::BuildD3DRes(D3D12Viewer *viewer, CD3DX12_CPU_DESCRIPTOR_HANDLE &cbvCPUHandle, CD3DX12_GPU_DESCRIPTOR_HANDLE &cbvGPUHandle)
{
	ID3D12Device *device = viewer->GetDevice();
	UINT32 handleOffset = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	m_illumGlobalConstantBufferSize = (sizeof(IllumGlobalConstants) + 255) & ~255;

	ThrowIfFailed(device->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(m_illumGlobalConstantBufferSize * D3D12Viewer::FrameCount),
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&m_illumGlobalConstantBuffer)));
	NAME_D3D12_OBJECT(m_illumGlobalConstantBuffer);

	CD3DX12_RANGE readRange(0, 0);		// We do not intend to read from this resource on the CPU.
	ThrowIfFailed(m_illumGlobalConstantBuffer->Map(0, &readRange, reinterpret_cast<void**>(&m_pIllumGlobalConstants)));

	m_lightSourceConstantBufferSize = (sizeof(LightSourceConstants) + 255) & ~255;

	//if (m_lightSourceCount > 0)
	{
		ThrowIfFailed(device->CreateCommittedResource(
			&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
			D3D12_HEAP_FLAG_NONE,
			&CD3DX12_RESOURCE_DESC::Buffer(m_lightSourceConstantBufferSize * m_lightSourceCountX * D3D12Viewer::FrameCount),
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			IID_PPV_ARGS(&m_lightSourceConstantBuffer)));
		NAME_D3D12_OBJECT(m_lightSourceConstantBuffer);

		ThrowIfFailed(m_lightSourceConstantBuffer->Map(0, &readRange, reinterpret_cast<void**>(&m_pLightSourceConstants)));
	}

	// Create a CBV for each frame.
	UINT64 cbOffset0 = 0;
	m_IllumCbvHandles = new CD3DX12_GPU_DESCRIPTOR_HANDLE[D3D12Viewer::FrameCount];
	for (UINT32 n = 0; n < D3D12Viewer::FrameCount; n++)
	{
		// Describe and create a constant buffer view (CBV).
		{
			D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc = {};
			cbvDesc.BufferLocation = m_illumGlobalConstantBuffer->GetGPUVirtualAddress() + cbOffset0;
			cbvDesc.SizeInBytes = m_illumGlobalConstantBufferSize;
			cbOffset0 += m_illumGlobalConstantBufferSize;
			device->CreateConstantBufferView(&cbvDesc, cbvCPUHandle);
			m_IllumCbvHandles[n] = cbvGPUHandle;
			cbvGPUHandle.Offset(handleOffset);
			cbvCPUHandle.Offset(handleOffset);
		}

		UINT64 cbOffset1 = n * m_lightSourceConstantBufferSize * m_lightSourceCountX;
		for (UINT32 l = 0; l < m_lightSourceCountX; ++l)
		{
			D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc = {};
			cbvDesc.BufferLocation = m_lightSourceConstantBuffer->GetGPUVirtualAddress() + cbOffset1;
			cbvDesc.SizeInBytes = m_lightSourceConstantBufferSize;
			cbOffset1 += m_lightSourceConstantBufferSize;
			device->CreateConstantBufferView(&cbvDesc, cbvCPUHandle);
			cbvGPUHandle.Offset(handleOffset);
			cbvCPUHandle.Offset(handleOffset);
		}
	}
}

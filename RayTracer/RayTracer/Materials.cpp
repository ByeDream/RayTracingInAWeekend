#include "stdafx.h"
#include "Materials.h"

#include "Ray.h"
#include "Hitables.h"
#include "Randomizer.h"
#include "Optics.h"

#include "SimpleTexture2D.h"
#include "D3D12Viewer.h"
#include "D3D12Helper.h"

Lambertian::Lambertian(const ITexture2D *albedo)
	: m_albedo(albedo)
{

}

BOOL Lambertian::Scatter(const Ray &r_in, const HitRecord &rec, Vec3 &attenuation, Ray &r_scattered, Vec3 &emitted) const
{
	// For simplicity,  scatter always and attenuate by its reflectance R, 
	// Or it can scatter with no attenuation but absorb the fraction 1 - R of the rays
	// Or it could be a mixture of those strategies, like only scatter with some probability p and have attenuation be albedo / p


	// Scatter a ray back to the air with a random direction from the unit radius sphere that is tangent to the hitpoint
	// recursively sample the indirect light with absorb half the energy(50% reflectors), until reach the sky light 
	Vec3 target = rec.m_position + rec.m_normal + Randomizer::RomdomInUnitSphere();
	r_scattered = Ray(rec.m_position, target - rec.m_position);
	attenuation = m_albedo->Sample(rec.m_u, rec.m_v);
	return TRUE; // always
}


void Lambertian::ApplySRV(D3D12Viewer *viewer) const
{
	ID3D12GraphicsCommandList *commandList = viewer->GetGraphicsCommandList();
	commandList->SetGraphicsRootDescriptorTable(2, m_albedo->m_d3dRes.m_SRVHandle);
}

void Lambertian::ApplyCBV(D3D12Viewer *viewer, D3D12_GPU_DESCRIPTOR_HANDLE illumCbvHandle) const
{
	ID3D12GraphicsCommandList *commandList = viewer->GetGraphicsCommandList();
	commandList->SetGraphicsRootDescriptorTable(1, illumCbvHandle);
}

PipelineState Lambertian::s_pso;

void Lambertian::BuildPSO(D3D12Viewer *viewer)
{
	D3D12_INPUT_LAYOUT_DESC inputLayout{ SimpleMesh::D3DVertexDeclaration, SimpleMesh::D3DVertexDeclarationElementCount };

	CD3DX12_DESCRIPTOR_RANGE1 ranges[3];
	ranges[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 0, 0, D3D12_DESCRIPTOR_RANGE_FLAG_DATA_STATIC);
	ranges[1].Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 2, 0, D3D12_DESCRIPTOR_RANGE_FLAG_DATA_STATIC);
	ranges[2].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0, 0, D3D12_DESCRIPTOR_RANGE_FLAG_DATA_STATIC);

	CD3DX12_ROOT_PARAMETER1 rootParameters[3];
	rootParameters[0].InitAsDescriptorTable(1, &ranges[0], D3D12_SHADER_VISIBILITY_VERTEX);
	rootParameters[1].InitAsDescriptorTable(1, &ranges[1], D3D12_SHADER_VISIBILITY_PIXEL);
	rootParameters[2].InitAsDescriptorTable(1, &ranges[2], D3D12_SHADER_VISIBILITY_PIXEL);

	D3D12_STATIC_SAMPLER_DESC sampler = {};
	sampler.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
	sampler.AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	sampler.AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	sampler.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	sampler.MipLODBias = 0;
	sampler.MaxAnisotropy = 0;
	sampler.ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
	sampler.BorderColor = D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK;
	sampler.MinLOD = 0.0f;
	sampler.MaxLOD = D3D12_FLOAT32_MAX;
	sampler.ShaderRegister = 0;
	sampler.RegisterSpace = 0;
	sampler.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

	CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC rootSignatureDesc;
	rootSignatureDesc.Init_1_1(_countof(rootParameters), rootParameters, 1, &sampler, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

	viewer->CreatePipelineState(&s_pso, rootSignatureDesc, L"..\\Assets\\sceneGeometry_vs.hlsl", L"..\\Assets\\lambertian.hlsl", inputLayout, TRUE, TRUE, TRUE, FALSE);
}

void Lambertian::ApplyPSO(D3D12Viewer *viewer)
{
	ID3D12GraphicsCommandList *commandList = viewer->GetGraphicsCommandList();
	commandList->SetPipelineState(s_pso.m_PSO.Get());
	commandList->SetGraphicsRootSignature(s_pso.m_RS.Get());
}


Metal::Metal(ITexture2D *albedo, float fuzziness)
	: m_albedo(albedo)
{
	m_data.m_fuzziness.x = (fuzziness < 1.0f) ? ((fuzziness >= 0.0f) ? fuzziness : 0.0f) : 1.0f;
}

BOOL Metal::Scatter(const Ray &r_in, const HitRecord &rec, Vec3 &attenuation, Ray &r_scattered, Vec3 &emitted) const
{
	Vec3 r_reflected;
	Optics::Reflect(normalize(r_in.m_dir), rec.m_normal, r_reflected);
	r_scattered = Ray(rec.m_position, r_reflected + m_data.m_fuzziness.x * Randomizer::RomdomInUnitSphere());
	attenuation = m_albedo->Sample(rec.m_u, rec.m_v);
	return (dot(r_scattered.m_dir, rec.m_normal) > 0); // absorb the scatter ray if it is below the surface
}

void Metal::ApplySRV(D3D12Viewer *viewer) const
{
	ID3D12GraphicsCommandList *commandList = viewer->GetGraphicsCommandList();
	commandList->SetGraphicsRootDescriptorTable(3, m_albedo->m_d3dRes.m_SRVHandle);
}

void Metal::ApplyCBV(D3D12Viewer *viewer, D3D12_GPU_DESCRIPTOR_HANDLE illumCbvHandle) const
{
	ID3D12GraphicsCommandList *commandList = viewer->GetGraphicsCommandList();
	commandList->SetGraphicsRootDescriptorTable(1, m_d3dRes.m_MtlCbvHandle);
	commandList->SetGraphicsRootDescriptorTable(2, illumCbvHandle);
}

PipelineState Metal::s_pso;

void Metal::BuildPSO(D3D12Viewer *viewer)
{
	D3D12_INPUT_LAYOUT_DESC inputLayout{ SimpleMesh::D3DVertexDeclaration, SimpleMesh::D3DVertexDeclarationElementCount };

	CD3DX12_DESCRIPTOR_RANGE1 ranges[4];
	ranges[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 0, 0, D3D12_DESCRIPTOR_RANGE_FLAG_DATA_STATIC);
	ranges[1].Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 1, 0, D3D12_DESCRIPTOR_RANGE_FLAG_DATA_STATIC);
	ranges[2].Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 2, 0, D3D12_DESCRIPTOR_RANGE_FLAG_DATA_STATIC);
	ranges[3].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0, 0, D3D12_DESCRIPTOR_RANGE_FLAG_DATA_STATIC);

	CD3DX12_ROOT_PARAMETER1 rootParameters[4];
	rootParameters[0].InitAsDescriptorTable(1, &ranges[0], D3D12_SHADER_VISIBILITY_VERTEX);
	rootParameters[1].InitAsDescriptorTable(1, &ranges[1], D3D12_SHADER_VISIBILITY_PIXEL);
	rootParameters[2].InitAsDescriptorTable(1, &ranges[2], D3D12_SHADER_VISIBILITY_PIXEL);
	rootParameters[3].InitAsDescriptorTable(1, &ranges[3], D3D12_SHADER_VISIBILITY_PIXEL);

	D3D12_STATIC_SAMPLER_DESC sampler = {};
	sampler.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
	sampler.AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	sampler.AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	sampler.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	sampler.MipLODBias = 0;
	sampler.MaxAnisotropy = 0;
	sampler.ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
	sampler.BorderColor = D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK;
	sampler.MinLOD = 0.0f;
	sampler.MaxLOD = D3D12_FLOAT32_MAX;
	sampler.ShaderRegister = 0;
	sampler.RegisterSpace = 0;
	sampler.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

	CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC rootSignatureDesc;
	rootSignatureDesc.Init_1_1(_countof(rootParameters), rootParameters, 1, &sampler, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

	viewer->CreatePipelineState(&s_pso, rootSignatureDesc, L"..\\Assets\\sceneGeometry_vs.hlsl", L"..\\Assets\\metal.hlsl", inputLayout, TRUE, TRUE, TRUE, FALSE);
}

void Metal::ApplyPSO(D3D12Viewer *viewer)
{
	ID3D12GraphicsCommandList *commandList = viewer->GetGraphicsCommandList();
	commandList->SetPipelineState(s_pso.m_PSO.Get());
	commandList->SetGraphicsRootSignature(s_pso.m_RS.Get());
}

Dielectric::Dielectric(float refractiveIndex)
{
	m_data.m_refractiveIndex.x = refractiveIndex;
}

BOOL Dielectric::Scatter(const Ray &r_in, const HitRecord &rec, Vec3 &attenuation, Ray &r_scattered, Vec3 &emitted) const
{
	Vec3 outward_normal;
	Vec3 uv = normalize(r_in.m_dir);
	

	float ni_over_nt;
	attenuation = Vec3(1.0f, 1.0f, 1.0f); // Dielectrics absorb nothing
	Vec3 r_refracted;
	float reflect_prob;
	float cosine;
	if (dot(uv, rec.m_normal) > 0) {
		// from internal to outside
		outward_normal = -rec.m_normal;
		ni_over_nt = m_data.m_refractiveIndex.x;  // device by air ref index(1.0f)
		cosine = m_data.m_refractiveIndex.x * dot(uv, rec.m_normal);
	}
	else
	{
		// from outside to internal
		outward_normal = rec.m_normal;
		ni_over_nt = 1.0f / m_data.m_refractiveIndex.x;  // device by air ref index(1.0f)
		cosine = -dot(uv, rec.m_normal);
	}

	if (Optics::Refract(uv, outward_normal, ni_over_nt, r_refracted))
	{
		reflect_prob = Optics::Schlick(cosine, m_data.m_refractiveIndex.x);
	}
	else
	{
		// total internal reflection
		reflect_prob = 1.0f;
	}

	if (Randomizer::RandomUNorm() < reflect_prob) {
		Vec3 r_reflected;
		Optics::Reflect(uv, rec.m_normal, r_reflected);
		r_scattered = Ray(rec.m_position, r_reflected);
	}
	else
	{
		r_scattered = Ray(rec.m_position, r_refracted);
	}
	return TRUE;
}

void Dielectric::ApplyCBV(D3D12Viewer *viewer, D3D12_GPU_DESCRIPTOR_HANDLE illumCbvHandle) const
{
	ID3D12GraphicsCommandList *commandList = viewer->GetGraphicsCommandList();
	commandList->SetGraphicsRootDescriptorTable(1, m_d3dRes.m_MtlCbvHandle);
	commandList->SetGraphicsRootDescriptorTable(2, illumCbvHandle);
}

PipelineState Dielectric::s_pso;

void Dielectric::BuildPSO(D3D12Viewer *viewer)
{
	D3D12_INPUT_LAYOUT_DESC inputLayout{ SimpleMesh::D3DVertexDeclaration, SimpleMesh::D3DVertexDeclarationElementCount };

	CD3DX12_DESCRIPTOR_RANGE1 ranges[3];
	ranges[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 0, 0, D3D12_DESCRIPTOR_RANGE_FLAG_DATA_STATIC);
	ranges[1].Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 1, 0, D3D12_DESCRIPTOR_RANGE_FLAG_DATA_STATIC);
	ranges[2].Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 2, 0, D3D12_DESCRIPTOR_RANGE_FLAG_DATA_STATIC);

	CD3DX12_ROOT_PARAMETER1 rootParameters[3];
	rootParameters[0].InitAsDescriptorTable(1, &ranges[0], D3D12_SHADER_VISIBILITY_VERTEX);
	rootParameters[1].InitAsDescriptorTable(1, &ranges[1], D3D12_SHADER_VISIBILITY_PIXEL);
	rootParameters[2].InitAsDescriptorTable(1, &ranges[2], D3D12_SHADER_VISIBILITY_PIXEL);

	CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC rootSignatureDesc;
	rootSignatureDesc.Init_1_1(_countof(rootParameters), rootParameters, 0, nullptr, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

	viewer->CreatePipelineState(&s_pso, rootSignatureDesc, L"..\\Assets\\sceneGeometry_vs.hlsl", L"..\\Assets\\dielectric.hlsl", inputLayout, TRUE, TRUE, FALSE, TRUE);
}

void Dielectric::ApplyPSO(D3D12Viewer *viewer)
{
	ID3D12GraphicsCommandList *commandList = viewer->GetGraphicsCommandList();
	commandList->SetPipelineState(s_pso.m_PSO.Get());
	commandList->SetGraphicsRootSignature(s_pso.m_RS.Get());
}

DiffuseLight::DiffuseLight(const Vec3 intensity)
{
	DirectX::XMStoreFloat4(&m_data.m_intensity, intensity.m_simd);
}

BOOL DiffuseLight::Scatter(const Ray &r_in, const HitRecord &rec, Vec3 &attenuation, Ray &r_scattered, Vec3 &emitted) const
{
	emitted = DirectX::XMLoadFloat4(&m_data.m_intensity);
	return FALSE; // no scattering but emitting
}

void DiffuseLight::ApplyCBV(D3D12Viewer *viewer, D3D12_GPU_DESCRIPTOR_HANDLE illumCbvHandle) const
{
	ID3D12GraphicsCommandList *commandList = viewer->GetGraphicsCommandList();
	commandList->SetGraphicsRootDescriptorTable(1, m_d3dRes.m_MtlCbvHandle);
}

PipelineState DiffuseLight::s_pso;

void DiffuseLight::BuildPSO(D3D12Viewer *viewer)
{
	D3D12_INPUT_LAYOUT_DESC inputLayout{ SimpleMesh::D3DVertexDeclaration, SimpleMesh::D3DVertexDeclarationElementCount };

	CD3DX12_DESCRIPTOR_RANGE1 ranges[2];
	ranges[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 0, 0, D3D12_DESCRIPTOR_RANGE_FLAG_DATA_STATIC);
	ranges[1].Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 1, 0, D3D12_DESCRIPTOR_RANGE_FLAG_DATA_STATIC);

	CD3DX12_ROOT_PARAMETER1 rootParameters[2];
	rootParameters[0].InitAsDescriptorTable(1, &ranges[0], D3D12_SHADER_VISIBILITY_VERTEX);
	rootParameters[1].InitAsDescriptorTable(1, &ranges[1], D3D12_SHADER_VISIBILITY_PIXEL);

	CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC rootSignatureDesc;
	rootSignatureDesc.Init_1_1(_countof(rootParameters), rootParameters, 0, nullptr, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

	viewer->CreatePipelineState(&s_pso, rootSignatureDesc, L"..\\Assets\\sceneGeometry_vs.hlsl", L"..\\Assets\\diffuseLight.hlsl", inputLayout, TRUE, TRUE, TRUE, FALSE);
}

void DiffuseLight::ApplyPSO(D3D12Viewer *viewer)
{
	ID3D12GraphicsCommandList *commandList = viewer->GetGraphicsCommandList();
	commandList->SetPipelineState(s_pso.m_PSO.Get());
	commandList->SetGraphicsRootSignature(s_pso.m_RS.Get());
}

void IMaterial::BuildD3DRes(D3D12Viewer *viewer, CD3DX12_CPU_DESCRIPTOR_HANDLE &cbvCPUHandle, CD3DX12_GPU_DESCRIPTOR_HANDLE &cbvGPUHandle)
{
	if (GetID() == MID_DIFFUSE_LIGHT)
	{
		int i = 0;
	}

	ID3D12Device *device = viewer->GetDevice();
	// mtl constant buffers.
	if (GetDataSize() > 0 && GetDataPtr() != nullptr)
	{
		UINT32 handleOffset = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		size_t bufferSize = (GetDataSize() + 255) & ~255;
		// Create an upload heap for the mtl constant buffers.
		ThrowIfFailed(device->CreateCommittedResource(
			&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
			D3D12_HEAP_FLAG_NONE,
			&CD3DX12_RESOURCE_DESC::Buffer(bufferSize),
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			IID_PPV_ARGS(&m_d3dRes.m_MtlConstantBuffer)));
		m_d3dRes.m_MtlConstantBuffer->SetName(L"MtlConstantBuffer");

		void *dataMapping = nullptr;
		CD3DX12_RANGE readRange(0, 0);		// We do not intend to read from this resource on the CPU.
		ThrowIfFailed(m_d3dRes.m_MtlConstantBuffer->Map(0, &readRange, &dataMapping));
		memcpy(dataMapping, GetDataPtr(), GetDataSize());
		m_d3dRes.m_MtlConstantBuffer->Unmap(0, nullptr);

		// Describe and create a constant buffer view (CBV).
		D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc = {};
		cbvDesc.BufferLocation = m_d3dRes.m_MtlConstantBuffer->GetGPUVirtualAddress();
		cbvDesc.SizeInBytes = (UINT32)bufferSize;
		device->CreateConstantBufferView(&cbvDesc, cbvCPUHandle);
		m_d3dRes.m_MtlCbvHandle = cbvGPUHandle;
		cbvCPUHandle.Offset(handleOffset);
		cbvGPUHandle.Offset(handleOffset);
	}
}

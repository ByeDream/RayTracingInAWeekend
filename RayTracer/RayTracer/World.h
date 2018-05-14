#pragma once

#include "Hitables.h"

class Mesh;
class IMaterial;
class Object;
class D3D12Viewer;
struct PipelineState;
class SimpleCamera;

enum MeshUniqueID
{
	MESH_ID_HIGH_POLYGON_SPHERE = 0,
	MESH_ID_MEDIUM_POLYGON_SPHERE,
	MESH_ID_LOW_POLYGON_SPHERE,

	MESH_ID_TOTAL_COUNT
};

enum MaterialUniqueID
{
	MATERIAL_ID_RANDOM_LAMBERTIAN_START = 0,
	MATERIAL_ID_RANDOM_LAMBERTIAN_COUNT = 50,

	MATERIAL_ID_RANDOM_METAL_START = MATERIAL_ID_RANDOM_LAMBERTIAN_START + MATERIAL_ID_RANDOM_LAMBERTIAN_COUNT,
	MATERIAL_ID_RANDOM_METAL_COUNT = 50,

	MATERIAL_ID_GROUND = MATERIAL_ID_RANDOM_METAL_START + MATERIAL_ID_RANDOM_METAL_COUNT,
	MATERIAL_ID_LAMBERTIAN,
	MATERIAL_ID_METAL,
	MATERIAL_ID_DIELECTRIC,

	MATERIAL_ID_TOTAL_COUNT
};

class World : public IHitable
{
public:
	World() = default;
	~World() = default;

	void									ConstructWorld();
	void									DeconstructWorld();

	void									OnUpdate(SimpleCamera *camera, float elapsedSeconds) override;
	void									OnRender(D3D12Viewer *viewer) const;
	virtual BOOL							Hit(const Ray &r, float t_min, float t_max, HitRecord &out_rec) const override;

	void									BuildD3DRes(D3D12Viewer *viewer);

	inline UINT32							GetFrameIndex() const { return m_CurrentCbvIndex; }
	static const Vec3						SkyLight;
	static const float						SkyLightBlender;
private:
	void									LoadMeshes();
	void									LoadMaterials();

	std::vector<Mesh *>						m_meshes;
	std::vector<IMaterial *>				m_materials;

	// sort by materials to avoid pipeline state switching
	std::vector<Object *>					m_lambertianObjects;
	PipelineState *							m_lambertianPipelineState{ nullptr };
	std::vector<Object *>					m_metalObjects;
	PipelineState *							m_metalPipelineState{ nullptr };
	std::vector<Object *>					m_dielectricObjects;
	PipelineState *							m_dielectricPipelineState{ nullptr };

	std::vector<Object *>					m_objects; // for ray tracing, don't really care about sorting
	UINT32									m_objectsCount{ 0 };

	ComPtr<ID3D12DescriptorHeap>			m_CbvHeap;
	UINT32									m_CurrentCbvIndex;

	////////////////////////
	// TODO Light, put them here at the moment
	// Only one direction light
	struct IllumConstants
	{
		XMFLOAT4	lightDirV;				// Light direction in view space.
		XMFLOAT4	lightIntensity;
		XMFLOAT4	ambientIntensity;
	};

	UINT8 *									m_pIllumConstants;
	ComPtr<ID3D12Resource>					m_IllumConstantBuffer;
	UINT32									m_IllumConstantBufferSize;
	CD3DX12_GPU_DESCRIPTOR_HANDLE *			m_IllumCbvHandles;
public:
	D3D12_GPU_DESCRIPTOR_HANDLE				GetIllumCbvHandle(UINT32 index) {
		return m_IllumCbvHandles[index];
	}
	////////////////////////
};
#pragma once

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

	MATERIAL_ID_LAMBERTIAN0 = MATERIAL_ID_RANDOM_METAL_START + MATERIAL_ID_RANDOM_METAL_COUNT,
	MATERIAL_ID_LAMBERTIAN1,
	MATERIAL_ID_LAMBERTIAN2,
	MATERIAL_ID_LAMBERTIAN3,
	MATERIAL_ID_LAMBERTIAN4,
	MATERIAL_ID_METAL,
	MATERIAL_ID_DIELECTRIC,

	MATERIAL_ID_IMAGE_BASED_GROUND_SOIL,
	MATERIAL_ID_IMAGE_BASED_METAL_CHECKER,

	MATERIAL_ID_LIGHTSOURCE_WHITE,
	MATERIAL_ID_LIGHTSOURCE_BRIGHT,
	MATERIAL_ID_LIGHTSOURCE_RED,
	MATERIAL_ID_LIGHTSOURCE_GREEN,
	MATERIAL_ID_LIGHTSOURCE_BLUE,

	MATERIAL_ID_TOTAL_COUNT
};

class Mesh;
class IMaterial;
class ITexture2D;
class D3D12Viewer;

class Resources
{
public:
	void									Load();
	void									Unload();

	void									BuildD3DRes(D3D12Viewer *viewer, CD3DX12_CPU_DESCRIPTOR_HANDLE &CPUHandle, CD3DX12_GPU_DESCRIPTOR_HANDLE &GPUHandle);

	Mesh *									GetTheMesh(MeshUniqueID id) const;
	IMaterial *								GetTheMaterial(MaterialUniqueID id) const;
	inline size_t							GetMeshesCount() const { return m_meshes.size(); }
	inline size_t							GetTexturesCount() const { return m_textures.size(); }
	inline size_t							GetMaterialsCount() const { return m_materials.size(); }

private:
	void									LoadMeshes();
	void									LoadMaterials();

	std::vector<Mesh *>						m_meshes;
	std::vector<ITexture2D *>				m_textures;
	std::vector<IMaterial *>				m_materials;
};
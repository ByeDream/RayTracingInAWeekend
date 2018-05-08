#pragma once

#include "Hitables.h"

class Mesh;
class IMaterial;
class Object;

enum MeshUniqueID
{
	MESH_ID_HIGH_POLYGON_SPHERE = 0,
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

	virtual BOOL							Hit(const Ray &r, float t_min, float t_max, HitRecord &out_rec) const override;


private:
	void									LoadMeshes();
	void									LoadMaterials();

	std::vector<Mesh *>						m_meshes;
	std::vector<IMaterial *>				m_materials;
	std::vector<Object *>					m_objects;
};
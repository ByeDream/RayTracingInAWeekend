#pragma once

class Resources;
class D3D12Viewer;
class SimpleCamera;
class SimpleObjectBVHNode;
class LightSources;

enum WorldID
{
	WORLD_ID_RANDOM_SPHERES = 0,
	WORLD_ID_CORNELL_BOX,
};

class World
{
public:
	World() = default;
	~World() = default;

	void									ConstructWorld(WorldID wid, SimpleCamera *camera);
	void									DeconstructWorld();

	void									OnUpdate(SimpleCamera *camera, float elapsedSeconds);
	void									OnRender(D3D12Viewer *viewer) const;

	void									BuildD3DRes(D3D12Viewer *viewer);
	SimpleObjectBVHNode	*					GetObjectBVHTree() const { return m_objectBVHTree; }

	inline UINT32							GetFrameIndex() const { return m_CurrentCbvIndex; }
	inline LightSources *					GetLightSources() const { return m_lightSources; }

private:
	Resources *								m_resources;

	// TODO : Do we need a separate render ?
	// sort by materials to avoid pipeline state switching
	SimpleObjectBVHNode	*					m_objectBVHTree{ nullptr };
	size_t									m_objectsCount{ 0 };
	LightSources *							m_lightSources;

	ComPtr<ID3D12DescriptorHeap>			m_SRVHeap;
	UINT32									m_CurrentCbvIndex;
};
#pragma once

/** @brief Controls whether Build*Mesh functions generate vertices and/or indices, and what optimizations are performed while generating indices.
*/
enum BuildMeshMode {
	kBuildVerticesOnly = -1,	///< only build SimpleMesh::m_vertexBuffer and fill in vertex related SimpleMesh data; do not fill out any index related SimpleMesh elements
	kBuildUnoptimizedIndices = 0,	///< only build SimpleMesh::m_indexBuffer with no optimization for reuse cache and fill in index related SimpleMesh data; do not fill out any vertex related SimpleMesh elements
	kBuildVerticesAndUnoptimizedIndices = 1,	///< build SimpleMesh::m_vertexBuffer and SimpleMesh::m_indexBuffer with no optimization for reuse cache
	kBuildOptimizedIndices = 2,	///< build SimpleMesh::m_indexBuffer striped to maximize vertex reuse; do not fill out any vertex related SimpleMesh elements
	kBuildVerticesAndOptimizedIndices = 3,	///< build SimpleMesh::m_vertexBuffer and SimpleMesh::m_indexBuffer striped to maximize vertex reuse
	kBuildDispatchDrawOptimizedIndices = 4,	///< build SimpleMesh::m_indexBuffer striped to maximize vertex reuse in dispatch draw; do not fill out any vertex related SimpleMesh elements
	kBuildVerticesAndDispatchDrawOptimizedIndices = 5,	///< build SimpleMesh::m_vertexBuffer and SimpleMesh::m_indexBuffer striped to maximize vertex reuse in dispatch draw
};

class SimpleMesh;

enum IndexSize
{
	kIndexSize16,
	kIndexSize32
};

enum PrimitiveType
{
	kPrimitiveTypeTriList,
};

struct SimpleMeshVertex
{
	XMFLOAT3 m_position;
	XMFLOAT3 m_normal;
	XMFLOAT4 m_tangent;
	XMFLOAT4 m_color;
	XMFLOAT2 m_texture;
};

class Mesh
{
public:
	UINT32 m_vertexCount;
	UINT32 m_vertexAttributeCount;
	UINT32 m_vertexBufferSize;
	UINT32 m_indexCount;
	void *m_vertexBuffer;

	IndexSize m_indexType;
	PrimitiveType m_primitiveType;
	UINT32 m_indexBufferSize;


	/*
	enum { kMaximumVertexBufferElements = 16 };

	
	uint8_t m_reserved0[4];
	void *m_indexBuffer;
	sce::Gnm::Buffer m_buffer[kMaximumVertexBufferElements];

	sce::Gnm::ResourceHandle m_vertexBufferRH;
	sce::Gnm::ResourceHandle m_indexBufferRH;

	Matrix4 m_bufferToModel;

	Mesh();
	void SetVertexBuffer(sce::Gnmx::GnmxGfxContext &gfxc, sce::Gnm::ShaderStage stage);
	Vector4 getElement(uint32_t attributeIndex, uint32_t elementIndex) const;
	void setElement(uint32_t attributeIndex, uint32_t elementIndex, Vector4 value);

	void copyPositionAttribute(const Mesh *source, uint32_t attribute);
	void copyAttribute(const Mesh *source, uint32_t attribute);
	void getMinMaxFromAttribute(Vector4 *min, Vector4 *max, uint32_t attribute) const;

	void allocate(const sce::Gnm::DataFormat *dataFormat, uint32_t attributes, uint32_t elements, uint32_t indices, IAllocator* allocator);
	void compress(const sce::Gnm::DataFormat *dataFormat, const Framework::SimpleMesh *source, IAllocator* allocator);
	*/
};


class SimpleMesh : public Mesh
{
public:
	UINT32 m_vertexStride; // in bytes
	UINT32 m_reserved[3];
};

class SimpeMeshBuilder
{
public:
	/** @brief Creates a SimpleMesh in the shape of a torus.
	* @param eMode Controls whether vertices and/or indices are generated, and what optimizations are performed while generating indices.
	* @param destMesh The mesh to receive the torus shape
	* @param outerRadius The outer radius of the torus - the distance from its center to the center of its cross-section
	* @param innerRadius The inner radius of the torus - the distance from the center of its cross-section to its surface
	* @param outerQuads The number of quadrilaterals around the outer radius
	* @param innerQuads The number of quadrilaterals around the inner radius
	* @param outerRepeats The number of times that UV coordinates repeat around the outer radius
	* @param innerRepeats The number of times that UV coordinates repeat around the inner radius
	*/
	static void BuildTorusMesh(BuildMeshMode const eMode, SimpleMesh *destMesh, float outerRadius, float innerRadius, UINT16 outerQuads, UINT16 innerQuads, float outerRepeats, float innerRepeats);


	/** @brief Legacy version of BuildTorusMesh.
	*/
	static inline void BuildTorusMesh(SimpleMesh *destMesh, float outerRadius, float innerRadius, UINT16 outerQuads, UINT16 innerQuads, float outerRepeats, float innerRepeats)
	{
		BuildTorusMesh(kBuildVerticesAndUnoptimizedIndices, destMesh, outerRadius, innerRadius, outerQuads, innerQuads, outerRepeats, innerRepeats);
	}

	/** @brief Creates a SimpleMesh in the shape of a sphere.
	* @param destMesh The mesh to receive the sphere shape.
	* @param radius The radius of the sphere
	* @param xdiv The number of X axis subdivisions
	* @param ydiv The number of Y axis subdivisions
	* @param tx The X coordinate of the sphere's center
	* @param ty The Y coordinate of the sphere's center
	* @param tz The Z coordinate of the sphere's center
	*/
	static void BuildSphereMesh(SimpleMesh *destMesh, float radius, long xdiv, long ydiv, float tx = 0.0f, float ty = 0.0f, float tz = 0.0f);

	/** @brief Creates a SimpleMesh in the shape of a sphere.
	* @param destMesh The mesh to receive the sphere shape.
	* @param radius The radius of the sphere
	* @param xdiv The number of X axis subdivisions
	* @param ydiv The number of Y axis subdivisions
	* @param tx The X coordinate of the sphere's center
	* @param ty The Y coordinate of the sphere's center
	* @param tz The Z coordinate of the sphere's center
	*/
	static void BuildSphereMesh(const char *name, SimpleMesh *destMesh, float radius, long xdiv, long ydiv, float tx = 0.0f, float ty = 0.0f, float tz = 0.0f);

	/** @brief Creates a SimpleMesh in the shape of a quadrilateral.
	* @param destMesh The mesh to receive the quadrilateral shape
	* @param size Half the length of a quadrilateral edge
	*/
	static void BuildQuadMesh(SimpleMesh *destMesh, float size);

	/** @brief Creates a SimpleMesh in the shape of a quadrilateral.
	* @param destMesh The mesh to receive the quadrilateral shape
	* @param size Half the length of a quadrilateral edge
	*/
	static void BuildQuadMesh(const char *name, SimpleMesh *destMesh, float size);

	/** @brief Creates a SimpleMesh in the shape of a cube.
	* @param destMesh The mesh to receive the cube shape
	* @param side The length of one side of the cube
	*/
	static void BuildCubeMesh(SimpleMesh *destMesh, float side);

	/** @brief Creates a SimpleMesh in the shape of a cube.
	* @param destMesh The mesh to receive the cube shape
	* @param side The length of one side of the cube
	*/
	static void BuildCubeMesh(const char *name, SimpleMesh *destMesh, float side);

	/** @brief Computes a height map scale by analyzing a mesh's contents
	*/
	static float ComputeMeshSpecificBumpScale(const SimpleMesh *srcMesh);

	/** @brief Saves a SimpleMesh to a binary file.
	* @param simplemesh A pointer to the mesh to be saved
	* @param filename The path to the SimpleMesh file to save
	*/
	static void SaveSimpleMesh(const SimpleMesh* simplemesh, const char* filename);

	/** @brief Loads a SimpleMesh from a binary file (created with SaveSimpleMesh).  The vertex data will
	*         be stored in shared VRAM, and must be freed by the caller when the SimpleMesh is no longer required.
	* @param simpleMesh If the load is successful, the new SimpleMesh object will be written here.
	* @param filename The path to the SimpleMesh file to load
	*/
	static void LoadSimpleMesh(SimpleMesh* simpleMesh, const char* filename);

	/** @brief Loads a SimpleMesh from a binary file (created with SaveSimpleMesh).  The vertex data will
	*         be stored in shared VRAM, and must be freed by the caller when the SimpleMesh is no longer required.
	* @param simpleMesh If the load is successful, the new SimpleMesh object will be written here.
	* @param filename The path to the SimpleMesh file to load
	*/
	static void LoadSimpleMesh(SimpleMesh* simpleMesh, const char* filename);

	/** @brief Scales the vertex positions of a SimpleMesh, without affecting any other vertex attributes.
	* @param simpleMesh This is the SimpleMesh object to scale
	* @param scale This is the scale to apply to the vertex positions of the SimpleMesh.
	*/
	static void scaleSimpleMesh(SimpleMesh* simpleMesh, float scale);

};
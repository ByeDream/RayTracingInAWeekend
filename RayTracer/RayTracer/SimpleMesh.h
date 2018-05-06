#pragma once


Vector4 getBufferElement(sce::Gnm::Buffer buffer, uint32_t elementIndex);
void setBufferElement(sce::Gnm::Buffer buffer, uint32_t elementIndex, Vector4 value);
void getBufferElementMinMax(Vector4 *min, Vector4 *max, sce::Gnm::Buffer buffer);

class SimpleMesh;

class Mesh
{
public:
	enum { kMaximumVertexBufferElements = 16 };

	void *m_vertexBuffer;
	uint32_t m_vertexBufferSize;
	uint32_t m_vertexCount;
	uint32_t m_vertexAttributeCount;
	uint8_t m_reserved0[4];
	void *m_indexBuffer;
	uint32_t m_indexBufferSize;
	uint32_t m_indexCount;
	sce::Gnm::IndexSize m_indexType;
	sce::Gnm::PrimitiveType m_primitiveType;
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
};

void tessellate(Mesh *destination, const Mesh *source, IAllocator* allocator);

class SimpleMesh : public Mesh
{
public:
	uint32_t m_vertexStride; // in bytes
	uint32_t m_reserved[3];
};

/** @brief Describes a vertex format element for a SimpleMesh
*/
enum MeshVertexBufferElement
{
	kPosition,
	kNormal,
	kTangent,
	kColor,
	kTexture
};

/** @brief Sets the vertex format for a SimpleMesh from an array of MeshVertexBufferElements
*/
void SetMeshVertexBufferFormat(sce::Gnm::Buffer* dest, SimpleMesh *destMesh, const MeshVertexBufferElement* element, uint32_t elements);

/** @brief Sets a standard vertex format that contains all possible MeshVertexBufferElements
*/
void SetMeshVertexBufferFormat(SimpleMesh *destMesh);


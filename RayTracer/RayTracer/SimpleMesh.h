#pragma once

enum IndexSize
{
	kIndexSize16,
	kIndexSize32
};

enum PrimitiveType
{
	kPrimitiveTypeTriList,
	// TODO more
};

class Mesh
{
public:
	UINT32 m_vertexCount;
	UINT32 m_vertexAttributeCount;
	UINT32 m_vertexBufferSize;
	void *m_vertexBuffer;

	UINT32 m_indexCount;
	IndexSize m_indexType;
	UINT32 m_indexBufferSize;
	void *m_indexBuffer;

	PrimitiveType m_primitiveType;

	Mesh()
		: m_vertexBuffer(0)
		, m_vertexBufferSize(0)
		, m_vertexCount(0)
		, m_vertexAttributeCount(0)
		, m_indexBuffer(0)
		, m_indexBufferSize(0)
		, m_indexCount(0)
		, m_indexType(kIndexSize16)
		, m_primitiveType(kPrimitiveTypeTriList)
	{}
};


// The Simple Mesh
struct SimpleMeshVertex
{
	XMFLOAT3 m_position;
	XMFLOAT3 m_normal;
	XMFLOAT4 m_tangent;
	XMFLOAT2 m_texture;
};

enum SimpleMeshVertexBufferElement
{
	kPosition = 0,
	kNormal,
	kTangent,
	kTexture,

	kSimpleMeshVertexBufferElementCount
};

class SimpleMesh : public Mesh
{
public:
	UINT32 m_vertexStride; // in bytes
};
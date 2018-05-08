#pragma once

class D3D12Viewer;

enum IndexSize
{
	kIndexSize16,
	kIndexSize32
};

enum PrimitiveType
{
	kPrimitiveTypePoints,
	kPrimitiveTypeLineList,
	kPrimitiveTypeLineStrip,
	kPrimitiveTypeTriList,
	kPrimitiveTypeTriStrip,
};

struct MeshD3D12Resources
{
	ComPtr<ID3D12Resource>			m_vertexBufferHeap;
	D3D12_VERTEX_BUFFER_VIEW		m_vertexBufferView;
	ComPtr<ID3D12Resource>			m_indexBufferHeap;
	D3D12_INDEX_BUFFER_VIEW			m_indexBufferView;
};

class Mesh
{
public:
	UINT32 m_vertexCount;
	UINT32 m_vertexAttributeCount;
	UINT32 m_vertexBufferSize;
	void *m_vertexBuffer;
	UINT32 m_vertexStride; // in bytes

	UINT32 m_indexCount;
	IndexSize m_indexType;
	UINT32 m_indexBufferSize;
	void *m_indexBuffer;

	PrimitiveType m_primitiveType;

	MeshD3D12Resources m_d3dRes;

	Mesh()
		: m_vertexBuffer(0)
		, m_vertexBufferSize(0)
		, m_vertexCount(0)
		, m_vertexAttributeCount(0)
		, m_vertexStride(0)
		, m_indexBuffer(0)
		, m_indexBufferSize(0)
		, m_indexCount(0)
		, m_indexType(kIndexSize16)
		, m_primitiveType(kPrimitiveTypeTriList)
	{}

	virtual ~Mesh() = default;

	virtual void BuildD3DRes(D3D12Viewer *viewer);
};

// The Simple Mesh
struct SimpleMeshVertex
{
	XMFLOAT3 m_position;
	XMFLOAT3 m_normal;
	XMFLOAT4 m_tangent;
	XMFLOAT2 m_texture;
};

class SimpleMesh : public Mesh
{
public:
	static D3D12_INPUT_ELEMENT_DESC D3DVertexDeclaration[];
	static UINT32 D3DVertexDeclarationElementCount;

	virtual ~SimpleMesh()
	{
		if (m_vertexBuffer)
			delete m_vertexBuffer;
		if (m_indexBuffer)
			delete m_vertexBuffer;
	}
};
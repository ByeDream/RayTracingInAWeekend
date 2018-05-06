
#include "stdafx.h"
#include "simple_mesh.h"
//#include "dataformat_interpreter.h"
//#include "Memory/Allocators.h"

using namespace sce;

class SimpleMeshVertex
{
public:
	Vector3Unaligned m_position;
	Vector3Unaligned m_normal;
	Vector4Unaligned m_tangent;
	Vector4Unaligned m_color;
	Vector2Unaligned m_texture;
};

Framework::Mesh::Mesh()
	: m_vertexBuffer(0)
	, m_vertexBufferSize(0)
	, m_vertexCount(0)
	, m_vertexAttributeCount(0)
	, m_indexBuffer(0)
	, m_indexBufferSize(0)
	, m_indexCount(0)
	, m_indexType(sce::Gnm::kIndexSize16)
	, m_primitiveType(sce::Gnm::kPrimitiveTypeTriList)
	, m_bufferToModel(Matrix4::identity())
{
}

void Framework::Mesh::SetVertexBuffer(Gnmx::GnmxGfxContext &gfxc, Gnm::ShaderStage stage)
{
	SCE_GNM_ASSERT(m_vertexAttributeCount < kMaximumVertexBufferElements);
	gfxc.setVertexBuffers(stage, 0, m_vertexAttributeCount, m_buffer);
}

Vector4 Framework::getBufferElement(sce::Gnm::Buffer buffer, uint32_t elementIndex)
{
	SCE_GNM_ASSERT_MSG(elementIndex < buffer.getNumElements(), "can't ask for element %d in a mesh that has only %d", elementIndex, buffer.getNumElements());
	const Gnm::DataFormat dataFormat = buffer.getDataFormat();
	const uint32_t stride = buffer.getStride();
	const uint8_t *src = static_cast<const uint8_t *>(buffer.getBaseAddress()) + elementIndex * stride;
	Reg32 reg32[4];
	dataFormatDecoder(reg32, static_cast<const uint32_t *>(static_cast<const void *>(src)), dataFormat);
	Vector4 value;
	memcpy(&value, reg32, sizeof(value));
	return value;
}

void Framework::setBufferElement(sce::Gnm::Buffer buffer, uint32_t elementIndex, Vector4 value)
{
	SCE_GNM_ASSERT_MSG(elementIndex < buffer.getNumElements(), "can't ask for element %d in a mesh that has only %d", elementIndex, buffer.getNumElements());
	const Gnm::DataFormat dataFormat = buffer.getDataFormat();
	const uint32_t stride = buffer.getStride();
	uint8_t *dest = static_cast<uint8_t *>(buffer.getBaseAddress()) + elementIndex * stride;
	Reg32 reg32[4];
	memcpy(reg32, &value, sizeof(value));
	uint32_t destDwords = 0;
	uint32_t temp[4];
	dataFormatEncoder(temp, &destDwords, reg32, dataFormat);
	memcpy(dest, temp, dataFormat.getBytesPerElement());
}

void Framework::getBufferElementMinMax(Vector4 *min, Vector4 *max, sce::Gnm::Buffer buffer)
{
	*min = *max = getBufferElement(buffer, 0);
	for (uint32_t elementIndex = 1; elementIndex < buffer.getNumElements(); ++elementIndex)
	{
		const Vector4 element = getBufferElement(buffer, elementIndex);
		*min = minPerElem(*min, element);
		*max = maxPerElem(*max, element);
	}
}

Vector4 Framework::Mesh::getElement(uint32_t attributeIndex, uint32_t elementIndex) const
{
	SCE_GNM_ASSERT_MSG(attributeIndex < m_vertexAttributeCount, "can't ask for attribute %d in a mesh that has only %d", attributeIndex, m_vertexAttributeCount);
	const Gnm::Buffer buffer = m_buffer[attributeIndex];
	return getBufferElement(buffer, elementIndex);
}

void Framework::Mesh::setElement(uint32_t attributeIndex, uint32_t elementIndex, Vector4 value)
{
	SCE_GNM_ASSERT_MSG(attributeIndex < m_vertexAttributeCount, "can't ask for attribute %d in a mesh that has only %d", attributeIndex, m_vertexAttributeCount);
	const Gnm::Buffer buffer = m_buffer[attributeIndex];
	setBufferElement(buffer, elementIndex, value);
}

void Framework::Mesh::getMinMaxFromAttribute(Vector4 *min, Vector4 *max, uint32_t attributeIndex) const
{
	SCE_GNM_ASSERT_MSG(attributeIndex < m_vertexAttributeCount, "can't ask for attribute %d in a mesh that has only %d", attributeIndex, m_vertexAttributeCount);
	const Gnm::Buffer buffer = m_buffer[attributeIndex];
	getBufferElementMinMax(min, max, buffer);
}

void Framework::Mesh::allocate(const sce::Gnm::DataFormat *dataFormat, uint32_t attributes, uint32_t elements, uint32_t indices, IAllocator* allocator)
{
	m_vertexAttributeCount = attributes;
	m_vertexCount = elements;
	m_indexCount = indices;

	m_vertexBufferSize = 0;
	for (uint32_t attribute = 0; attribute < m_vertexAttributeCount; ++attribute)
	{
		m_vertexBufferSize += dataFormat[attribute].getBytesPerElement() * m_vertexCount;
		m_vertexBufferSize = (m_vertexBufferSize + 3) & ~3;
	}
	m_indexBufferSize = indices * ((m_indexType == sce::Gnm::kIndexSize16) ? sizeof(uint16_t) : sizeof(uint32_t));

	const uint32_t bufferSize = m_vertexBufferSize + m_indexBufferSize;

	m_vertexBuffer = allocator->allocate(sce::Gnm::SizeAlign(bufferSize, sce::Gnm::kAlignmentOfBufferInBytes));
	m_indexBuffer = (uint8_t *)m_vertexBuffer + m_vertexBufferSize;

	unsigned offset = 0;
	for (uint32_t attribute = 0; attribute < m_vertexAttributeCount; ++attribute)
	{
		m_buffer[attribute].initAsVertexBuffer((uint8_t *)m_vertexBuffer + offset, dataFormat[attribute], dataFormat[attribute].getBytesPerElement(), m_vertexCount);
		offset += dataFormat[attribute].getBytesPerElement() * m_vertexCount;
		offset = (offset + 3) & ~3;
	}
}

void Framework::SetMeshVertexBufferFormat(Gnm::Buffer* buffer, Framework::SimpleMesh *destMesh, const MeshVertexBufferElement* element, uint32_t elements)
{
	while (elements--)
	{
		switch (*element++)
		{
		case Framework::kPosition:
			buffer->initAsVertexBuffer(static_cast<uint8_t*>(destMesh->m_vertexBuffer) + offsetof(SimpleMeshVertex, m_position), Gnm::kDataFormatR32G32B32Float, sizeof(SimpleMeshVertex), destMesh->m_vertexCount);
			break;
		case Framework::kNormal:
			buffer->initAsVertexBuffer(static_cast<uint8_t*>(destMesh->m_vertexBuffer) + offsetof(SimpleMeshVertex, m_normal), Gnm::kDataFormatR32G32B32Float, sizeof(SimpleMeshVertex), destMesh->m_vertexCount);
			break;
		case Framework::kTangent:
			buffer->initAsVertexBuffer(static_cast<uint8_t*>(destMesh->m_vertexBuffer) + offsetof(SimpleMeshVertex, m_tangent), Gnm::kDataFormatR32G32B32A32Float, sizeof(SimpleMeshVertex), destMesh->m_vertexCount);
			break;
		case Framework::kColor:
			buffer->initAsVertexBuffer(static_cast<uint8_t*>(destMesh->m_vertexBuffer) + offsetof(SimpleMeshVertex, m_color), Gnm::kDataFormatR32G32B32A32Float, sizeof(SimpleMeshVertex), destMesh->m_vertexCount);
			break;
		case Framework::kTexture:
			buffer->initAsVertexBuffer(static_cast<uint8_t*>(destMesh->m_vertexBuffer) + offsetof(SimpleMeshVertex, m_texture), Gnm::kDataFormatR32G32Float, sizeof(SimpleMeshVertex), destMesh->m_vertexCount);
			break;
		}
		buffer->setResourceMemoryType(Gnm::kResourceMemoryTypeRO); // it's a vertex buffer, so read-only is OK
		++buffer;
	}
}

void Framework::SetMeshVertexBufferFormat(Framework::SimpleMesh *destMesh)
{
	const MeshVertexBufferElement element[] = { kPosition, kNormal, kTangent, kColor, kTexture };
	const uint32_t elements = sizeof(element) / sizeof(element[0]);
	SetMeshVertexBufferFormat(destMesh->m_buffer, destMesh, element, elements);
	destMesh->m_vertexAttributeCount = elements;
}


void SimpeMeshBuilder::Mesh::copyPositionAttribute(const Mesh *source, uint32_t attribute)
{
	SCE_GNM_ASSERT_MSG(m_vertexCount == source->m_vertexCount, "This mesh and the mesh it's copying from must have the same number of vertices.");
	Vector4 mini, maxi;
	source->getMinMaxFromAttribute(&mini, &maxi, 0);
	const Vector4 range = maxi - mini;
	const float x = range.getX();
	const float y = range.getY();
	const float z = range.getZ();
	const float w = range.getW();
	float scale = x;
	if (y > scale)
		scale = y;
	if (z > scale)
		scale = z;
	if (w > scale)
		scale = w;
	m_bufferToModel = Matrix4::translation(mini.getXYZ()) * Matrix4::scale(Vector3(scale));
	const float invScale = 1.f / scale;
	const Vector4 invTranslation = -mini * invScale;
	for (uint32_t element = 0; element < source->m_vertexCount; ++element)
	{
		Vector4 value = source->getElement(attribute, element);
		value = value * invScale + invTranslation;
		setElement(attribute, element, value);
	}
}

void SimpeMeshBuilder::Mesh::copyAttribute(const Mesh *source, uint32_t attribute)
{
	SCE_GNM_ASSERT_MSG(m_vertexCount == source->m_vertexCount, "This mesh and the mesh it's copying from must have the same number of vertices.");
	for (uint32_t element = 0; element < source->m_vertexCount; ++element)
	{
		Vector4 value = source->getElement(attribute, element);
		setElement(attribute, element, value);
	}
}

void SimpeMeshBuilder::Mesh::compress(const sce::Gnm::DataFormat *dataFormat, const SimpeMeshBuilder::SimpleMesh *source)
{
	m_primitiveType = source->m_primitiveType;
	m_indexType = source->m_indexType;
	allocate(dataFormat, source->m_vertexAttributeCount, source->m_vertexCount, source->m_indexCount, allocator);
	memcpy(m_indexBuffer, source->m_indexBuffer, m_indexBufferSize);
	copyPositionAttribute(source, 0);
	for (uint32_t attribute = 1; attribute < m_vertexAttributeCount; ++attribute)
		copyAttribute(source, attribute);
}

unsigned makeKey(unsigned a, unsigned b)
{
	if (a > b)
		std::swap(a, b);
	return (a << 16) | b;
}

class Vertex
{
public:
	Vector4 m_vector[4];
};

void SimpeMeshBuilder::tessellate(Mesh *destination, const Mesh *source)
{
	std::unordered_map<unsigned, Vertex> edgeToMidpoint;
	const unsigned oldIndices = source->m_indexCount;
	const unsigned oldTriangles = oldIndices / 3;
	const unsigned oldVertices = source->m_vertexCount;
	for (auto t = 0U; t < oldTriangles; t++)
	{
		unsigned i0, i1, i2;
		if (source->m_indexType == Gnm::kIndexSize16)
		{
			i0 = ((UINT16 *)source->m_indexBuffer)[t * 3 + 0];
			i1 = ((UINT16 *)source->m_indexBuffer)[t * 3 + 1];
			i2 = ((UINT16 *)source->m_indexBuffer)[t * 3 + 2];
		}
		else
		{
			i0 = ((uint32_t *)source->m_indexBuffer)[t * 3 + 0];
			i1 = ((uint32_t *)source->m_indexBuffer)[t * 3 + 1];
			i2 = ((uint32_t *)source->m_indexBuffer)[t * 3 + 2];
		}
		Vertex v[3];
		for (unsigned a = kPosition; a <= kTexture; ++a)
		{
			const Vector4 p0 = source->getElement(a, i0);
			const Vector4 p1 = source->getElement(a, i1);
			const Vector4 p2 = source->getElement(a, i2);
			v[0].m_vector[a] = (p0 + p1) * 0.5f;
			v[1].m_vector[a] = (p1 + p2) * 0.5f;
			v[2].m_vector[a] = (p2 + p0) * 0.5f;
		}
		edgeToMidpoint.insert({ makeKey(i0,i1),v[0] });
		edgeToMidpoint.insert({ makeKey(i1,i2),v[1] });
		edgeToMidpoint.insert({ makeKey(i2,i0),v[2] });
	}

	const unsigned newIndices = oldIndices * 4;
	const unsigned newVertices = source->m_vertexCount + edgeToMidpoint.size();
	const unsigned newIndexBufferSize = newIndices * ((source->m_indexType == Gnm::kIndexSize16) ? 2 : 4);

	Gnm::Buffer buffer[4];
	for (unsigned a = kPosition; a <= kTexture; ++a)
	{
		const unsigned newVertexBufferSize = newVertices * source->m_buffer[a].getDataFormat().getBytesPerElement();
		void *vertexBuffer = allocator->allocate(Gnm::SizeAlign(newVertexBufferSize, Gnm::kAlignmentOfBufferInBytes));
		buffer[a].initAsDataBuffer(vertexBuffer, source->m_buffer[a].getDataFormat(), newVertices);
		for (unsigned i = 0; i < oldVertices; ++i)
			setBufferElement(buffer[a], i, getBufferElement(source->m_buffer[a], i));
	}
	std::unordered_map<unsigned, unsigned> edgeToNewVertexIndex;
	unsigned nextVertexIndex = source->m_vertexCount;
	for (auto e : edgeToMidpoint)
	{
		edgeToNewVertexIndex.insert({ e.first,nextVertexIndex });
		for (unsigned a = kPosition; a <= kTexture; ++a)
			setBufferElement(buffer[a], nextVertexIndex, e.second.m_vector[a]);
		++nextVertexIndex;
	}
	destination->m_vertexCount = nextVertexIndex;

	void *indexBuffer = allocator->allocate(Gnm::SizeAlign(newIndexBufferSize, Gnm::kAlignmentOfBufferInBytes));
	for (auto t = 0U; t < oldTriangles; t++)
	{
		if (source->m_indexType == Gnm::kIndexSize16)
		{
			const UINT16 i0 = ((UINT16 *)source->m_indexBuffer)[t * 3 + 0];
			const UINT16 i1 = ((UINT16 *)source->m_indexBuffer)[t * 3 + 1];
			const UINT16 i2 = ((UINT16 *)source->m_indexBuffer)[t * 3 + 2];
			const UINT16 i[] =
			{
				i0,
				i1,
				i2,
				(UINT16)edgeToNewVertexIndex.find(makeKey(i0,i1))->second,
				(UINT16)edgeToNewVertexIndex.find(makeKey(i1,i2))->second,
				(UINT16)edgeToNewVertexIndex.find(makeKey(i2,i0))->second,
			};
			((UINT16 *)indexBuffer)[t * 12 + 0] = i[0];
			((UINT16 *)indexBuffer)[t * 12 + 1] = i[3];
			((UINT16 *)indexBuffer)[t * 12 + 2] = i[5];

			((UINT16 *)indexBuffer)[t * 12 + 3] = i[5];
			((UINT16 *)indexBuffer)[t * 12 + 4] = i[4];
			((UINT16 *)indexBuffer)[t * 12 + 5] = i[2];

			((UINT16 *)indexBuffer)[t * 12 + 6] = i[3];
			((UINT16 *)indexBuffer)[t * 12 + 7] = i[4];
			((UINT16 *)indexBuffer)[t * 12 + 8] = i[5];

			((UINT16 *)indexBuffer)[t * 12 + 9] = i[3];
			((UINT16 *)indexBuffer)[t * 12 + 10] = i[1];
			((UINT16 *)indexBuffer)[t * 12 + 11] = i[4];
		}
		else
		{
			const uint32_t i0 = ((uint32_t *)source->m_indexBuffer)[t * 3 + 0];
			const uint32_t i1 = ((uint32_t *)source->m_indexBuffer)[t * 3 + 1];
			const uint32_t i2 = ((uint32_t *)source->m_indexBuffer)[t * 3 + 2];
			const uint32_t i[] =
			{
				i0,
				i1,
				i2,
				(uint32_t)edgeToNewVertexIndex.find(makeKey(i0,i1))->second,
				(uint32_t)edgeToNewVertexIndex.find(makeKey(i1,i2))->second,
				(uint32_t)edgeToNewVertexIndex.find(makeKey(i2,i0))->second,
			};
			((uint32_t *)indexBuffer)[t * 12 + 0] = i[0];
			((uint32_t *)indexBuffer)[t * 12 + 1] = i[3];
			((uint32_t *)indexBuffer)[t * 12 + 2] = i[5];

			((uint32_t *)indexBuffer)[t * 12 + 3] = i[5];
			((uint32_t *)indexBuffer)[t * 12 + 4] = i[4];
			((uint32_t *)indexBuffer)[t * 12 + 5] = i[2];

			((uint32_t *)indexBuffer)[t * 12 + 6] = i[3];
			((uint32_t *)indexBuffer)[t * 12 + 7] = i[4];
			((uint32_t *)indexBuffer)[t * 12 + 8] = i[5];

			((uint32_t *)indexBuffer)[t * 12 + 9] = i[3];
			((uint32_t *)indexBuffer)[t * 12 + 10] = i[1];
			((uint32_t *)indexBuffer)[t * 12 + 11] = i[4];
		}
	}

	for (unsigned a = kPosition; a <= kTexture; ++a)
		destination->m_buffer[a] = buffer[a];

	destination->m_indexBuffer = indexBuffer;
	destination->m_vertexBuffer = destination->m_buffer[kPosition].getBaseAddress();
	destination->m_indexCount = newIndices;
	destination->m_indexBufferSize = newIndexBufferSize;
	destination->m_vertexBufferSize = destination->m_buffer[kPosition].getSize();
	destination->m_vertexAttributeCount = source->m_vertexAttributeCount;
	destination->m_indexType = source->m_indexType;
}


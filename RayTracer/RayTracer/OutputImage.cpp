#include "stdafx.h"
#include "OutputImage.h"
#include "PPMImageMaker.h"
#include "Vec3.h"

OutputImage::OutputImage(UINT32 width, UINT32 height, const char *name)
	: m_width(width)
	, m_height(height)
	, m_name(name)
{
	m_aspectRatio = static_cast<float>(width) / static_cast<float>(height);

	m_dataSizeInByte = (UINT64)m_width * (UINT64)m_height * (UINT64)m_pixelSizeInByte;
	if (m_dataSizeInByte)
		m_data = new UINT8[m_dataSizeInByte];
	memset(m_data, 0, m_dataSizeInByte);
}


OutputImage::~OutputImage()
{
	if (m_data)
	{
		delete[] m_data;
		m_data = nullptr;
	}
}

void OutputImage::RenderAsRainbow()
{
	for (UINT32 j = 0; j < m_height; j++)
	{
		for (UINT32 i = 0; i < m_width; i++)
		{
			Vec3 col(float(i) / float(m_width - 1), 1.0f - float(j) / float(m_height - 1), 0.2f);
			float a = 1.0f;

			UINT8 *baseOffset = m_data + (j * m_width + i) * m_pixelSizeInByte;
			*baseOffset = static_cast<UINT32>(col.r() * 255.0f) & 0xFF;
			*(baseOffset + 1) = static_cast<UINT32>(col.g() * 255.0f) & 0xFF;
			*(baseOffset + 2) = static_cast<UINT32>(col.b() * 255.0f) & 0xFF;
			*(baseOffset + 3) = static_cast<UINT32>(a * 255.0f) & 0xFF;
		}
	}

	m_isDirty = TRUE;
}

void OutputImage::RenderAsRed()
{
	for (UINT32 i = 0; i < m_width * m_height; i++)
	{
		UINT32 *baseOffset = reinterpret_cast<UINT32 *>(m_data + i * m_pixelSizeInByte);
		*(baseOffset) = 0xFF0000FF;
	}

	m_isDirty = TRUE;
}

void OutputImage::Render(const Vec3 *pixels, UINT32 pixelCount)
{
	if (pixelCount)
	{
		bool checkSize = pixelCount == (m_width * m_height);
		assert(checkSize && "unmatching image size and data size");
	}

	for (UINT32 j = 0; j < m_height; j++)
	{
		for (UINT32 i = 0; i < m_width; i++)
		{
			UINT32 index = j * m_width + i;
			const Vec3 &col = pixels[index];
			float a = 1.0f;

			UINT8 *baseOffset = m_data + index * m_pixelSizeInByte;
			*baseOffset = static_cast<UINT32>(col.r() * 255.0f) & 0xFF;
			*(baseOffset + 1) = static_cast<UINT32>(col.g() * 255.0f) & 0xFF;
			*(baseOffset + 2) = static_cast<UINT32>(col.b() * 255.0f) & 0xFF;
			*(baseOffset + 3) = static_cast<UINT32>(a * 255.0f) & 0xFF;
		}
	}

	m_isDirty = TRUE;
}

void OutputImage::Output()
{
	std::string ppmFileName = m_name + ".ppm";
	PPMImageMaker::OutputRGBA8ToFile(ppmFileName.c_str(), m_width, m_height, m_data, m_dataSizeInByte);
}
#include "stdafx.h"
#include "OutputImage.h"

OutputImage::OutputImage(unsigned width, unsigned height, const char *name)
	: m_width(width)
	, m_height(height)
	, m_name(name)
{
	m_dataSizeInByte = m_width * m_height * m_pixelSizeInByte;
	if (m_dataSizeInByte)
		m_data = new unsigned char[m_dataSizeInByte];
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

void OutputImage::InitAsRainbow()
{
	for (unsigned j = 0; j < m_height; j++)
	{
		for (unsigned i = 0; i < m_width; i++)
		{
			float r = float(i) / float(m_width - 1);
			float g = 1.0f - float(j) / float(m_height - 1);
			float b = 0.2f;
			float a = 1.0f;

			unsigned char *baseOffset = m_data + (j * m_width + i) * m_pixelSizeInByte;
			*baseOffset = static_cast<unsigned>(r * 255.0f) & 0xFF;
			*(baseOffset + 1) = static_cast<unsigned>(g * 255.0f) & 0xFF;
			*(baseOffset + 2) = static_cast<unsigned>(b * 255.0f) & 0xFF;
			*(baseOffset + 3) = static_cast<unsigned>(a * 255.0f) & 0xFF;
		}
	}
}

void OutputImage::InitAsRed()
{
	for (unsigned i = 0; i < m_width * m_height; i++)
	{
		unsigned *baseOffset = reinterpret_cast<unsigned *>(m_data + i * m_pixelSizeInByte);
		*(baseOffset) = 0xFF0000FF;
	}
}

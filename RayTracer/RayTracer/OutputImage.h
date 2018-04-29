#pragma once

//RGBA8 bitmap
class OutputImage
{
public:
	OutputImage(unsigned width, unsigned height);
	~OutputImage();

	void			InitAsRainbow();
	void			InitAsRed();

	void			OutputToPPM();

	unsigned		m_width{ 0 };
	unsigned		m_height{ 0 };
	unsigned		m_dataSizeInByte{ 0 };
	unsigned		m_pixelSizeInByte{ 4 };
	unsigned char *	m_data{ nullptr };
};
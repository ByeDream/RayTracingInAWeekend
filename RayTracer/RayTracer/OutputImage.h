#pragma once

struct Vec3;

//RGBA8 bitmap
class OutputImage
{
public:
	OutputImage(UINT32 width, UINT32 height, const char *name);
	~OutputImage();

	void			RenderAsRainbow();
	void			RenderAsRed();
	void            Render(const Vec3 *pixels, UINT32 pixelCount = 0);

	void			Output();

	UINT32			m_width{ 0 };
	UINT32			m_height{ 0 };
	UINT64			m_dataSizeInByte{ 0 };
	UINT32			m_pixelSizeInByte{ 4 };
	UINT8 *			m_data{ nullptr };
	std::string		m_name{};
};
#include "stdafx.h"
#include "PPMImageMaker.h"

#include "shlwapi.h"
#pragma comment(lib,"shlwapi.lib")

using namespace std;

void PPMImageMaker::OutputRGBA8ToFile(const char *fileName, unsigned imageWidth, unsigned imageHeight, const unsigned char *rgba8PixelData, unsigned dataSizeInByte)
{
	assert(rgba8PixelData && "invaild data ptr");
	const unsigned pixelSize = 4; //rgba8 format
	if (dataSizeInByte)
	{
		bool checkSize = dataSizeInByte == (imageWidth * imageHeight * pixelSize);
		assert(checkSize && "unmatching image size and data size");
	}

	string path = "..\\Assets";
	if (!PathIsDirectory(path.c_str()))
	{
		::CreateDirectory(path.c_str(), NULL);
	}

	string filePath = path + "\\" + string(fileName);
	ofstream ofs(filePath.c_str());
	assert(ofs && "failed to open file stream");

	ofs << "P3\n" << imageWidth << " " << imageHeight << "\n255\n";
	for (unsigned i = 0; i < imageWidth * imageHeight; i++)
	{

		unsigned r = static_cast<unsigned>(*(rgba8PixelData + i * pixelSize));
		if (r > 255)
		{
			int xxx = 0;
		}
		unsigned g = static_cast<unsigned>(*(rgba8PixelData + i * pixelSize + 1));
		unsigned b = static_cast<unsigned>(*(rgba8PixelData + i * pixelSize + 2));
		ofs << r << " " << g << " " << b << "\n";
	}

	ofs.close();
}
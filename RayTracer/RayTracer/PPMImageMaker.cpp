#include "stdafx.h"
#include "PPMImageMaker.h"

#include "shlwapi.h"
#pragma comment(lib,"shlwapi.lib")

using namespace std;

void PPMImageMaker::OutputRGBA8ToFile(const char *fileName, UINT32 imageWidth, UINT32 imageHeight, const UINT8 *rgba8PixelData, UINT64 dataSizeInByte)
{
	cout << "[PPMImageMaker] Save output image to PPM file: " << fileName << " ..." << endl;

	assert(rgba8PixelData && "invaild data ptr");
	const UINT32 pixelSize = 4; //rgba8 format
	if (dataSizeInByte)
	{
		bool checkSize = dataSizeInByte == ((UINT64)imageWidth * (UINT64)imageHeight * (UINT64)pixelSize);
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
	for (UINT32 i = 0; i < imageWidth * imageHeight; i++)
	{

		UINT32 r = static_cast<UINT32>(*(rgba8PixelData + i * pixelSize));
		if (r > 255)
		{
			int xxx = 0;
		}
		UINT32 g = static_cast<UINT32>(*(rgba8PixelData + i * pixelSize + 1));
		UINT32 b = static_cast<UINT32>(*(rgba8PixelData + i * pixelSize + 2));
		ofs << r << " " << g << " " << b << "\n";
	}

	ofs.close();

	cout << "[PPMImageMaker] Done" << endl;
}
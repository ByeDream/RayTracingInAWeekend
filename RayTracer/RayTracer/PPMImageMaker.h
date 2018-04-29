#pragma once

class PPMImageMaker
{
public:
	static void OutputRGBA8ToFile(const char *fileName, unsigned imageWidth, unsigned imageHeight, const unsigned char *rgbPixelData, unsigned dataSizeInByte = 0);
};

#pragma once

// generate a PPM image file that can be reviewed by :
// http://paulcuth.me.uk/netpbm-viewer/

class PPMImageMaker
{
public:
	static void OutputRGBA8ToFile(const char *fileName, unsigned imageWidth, unsigned imageHeight, const unsigned char *rgbPixelData, unsigned dataSizeInByte = 0);
};

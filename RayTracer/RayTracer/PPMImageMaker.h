#pragma once

// generate a PPM image file that can be reviewed by :
// http://paulcuth.me.uk/netpbm-viewer/

class PPMImageMaker
{
public:
	static void OutputRGBA8ToFile(const char *fileName, UINT32 imageWidth, UINT32 imageHeight, const UINT8 *rgbPixelData, UINT64 dataSizeInByte = 0);
};

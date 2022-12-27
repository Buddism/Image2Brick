#pragma once
#include "pch.h"

typedef int brickSizes;
typedef int brickItemIndex;
typedef vec2<unsigned int> PixelPos;

struct brickListItem
{
	int sizeX;
	int sizeY;
	std::string uiName;

	int volume = sizeX * sizeY;
	brickItemIndex id;
};

struct greedyListItem
{
	PixelPos pos{ 0, 0 };
	bool rotation = 0;
	uint8_t colorID = 0;
	brickItemIndex brickid;

	greedyListItem(PixelPos _pos, uint8_t _colorID);
};

struct PixelData
{
	uint8_t colorID = undefinedColorID;
	bool rotation = false;
	bool was_processed = false;

	const static int undefinedColorID = 254;
};

class GreedyBrick
{
public:
	GreedyBrick(const Image* _img, const std::vector<uint8_t>& colorIDPixels);
	std::vector<greedyListItem> greedyBrick();
private:
	std::vector<PixelData> pixels;
	const Image* img;

	void dump_processed();
	bool inBounds(int posX, int posY);
	bool testBrickFit(const unsigned int posX, const unsigned int posY, const unsigned int scaleX, const unsigned int scaleY, const uint8_t testColorID);
	void setBrickFitProccessed(const unsigned int posX, const unsigned int posY, const unsigned int scaleX, const unsigned int scaleY);
	brickItemIndex findLargestBrick(const unsigned int posX, const unsigned int posY, const int colorID, const bool rotation);
};

#pragma once
#include "pch.h"

typedef int brickSizes;
typedef uint16_t brickItemIndex;
typedef vec2<uint16_t> PixelPos;

struct brickListItem
{
	int sizeX;
	int sizeY;
	std::string uiName;

	int volume = sizeX * sizeY;
	brickItemIndex id;

	//lowest index of all bricks with this volume
	brickItemIndex minimalVolumeId;

	inline std::string toString()
	{
		return std::format("id({}): uiName: {}, sizeX: {}, sizeY: {}", id, uiName, sizeX, sizeY);
	}
};

struct greedyListItem
{
	bool rotation = 0;
	uint8_t colorID = 0;
	brickItemIndex brickid = 0;
	uint32_t totalPossibleStates = 0;

	PixelPos pos{ 0, 0 };

	std::string toString();
	greedyListItem(PixelPos _pos, uint8_t _colorID);
	greedyListItem() = default;
};


struct PixelData
{
	PixelPos pos{ 0, 0 };
	uint8_t colorID = undefinedColorID;
	bool rotation = false;
	bool hasBrick = false;

	brickItemIndex previousLargestBrickID[2];
	brickItemIndex previousLargestMinVolumeID[2];
	unsigned int nextUnsatisfiedPixel = 0;
	std::atomic<uint32_t> totalPossibleStates = 0;

	const static uint8_t maxColors = 64;
	const static uint8_t undefinedColorID = 0xFF;
	const static uint8_t satisifedColorID = 0xFE;
	const static uint8_t AlphaColorId     = 0xFD;
};

class GreedyBrick
{
public:
	GreedyBrick(const Image* _img, const std::vector<uint8_t>& colorIDPixels);
	std::vector<greedyListItem> greedyBrick();

private:
	std::vector<greedyListItem> outputGreedyList;
	PixelData* pixels;

	const Image* img;
	unsigned int width, height;

	void collapseBrick(greedyListItem& item, bool fullCollapse = false);
	void fullCollapseBrick(PixelData& pixel, bool rotation);

	bool testBrickFit(const unsigned int posX, const unsigned int posY, const unsigned int scaleX, const unsigned int scaleY, const uint8_t testColorID, uint32_t& totalPossibleStates);
	void calculateAllBrickStates(PixelData& pixel, const bool rotation);
	void getBestPossibleBrick(PixelData& pixel, greedyListItem& bestBrick, const bool rotation);
	greedyListItem getBestBrick();
};

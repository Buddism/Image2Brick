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

	inline std::string toString()
	{
		return std::format("id({}): uiName: {}, sizeX: {}, sizeY: {}", id, uiName, sizeX, sizeY);
	}
};

struct greedyListItem
{
	bool isValid = true;
	PixelPos pos{ 0, 0 };
	bool rotation = 0;
	uint8_t colorID = 0;
	brickItemIndex brickid;

	greedyListItem(PixelPos _pos, uint8_t _colorID);
};



struct PixelData
{
	PixelPos pos{ 0, 0 };
	uint8_t colorID = undefinedColorID;
	bool rotation = false;
	bool was_processed = false;

	const static int undefinedColorID = 254;

	//unsigned long int possibleBrickStates = 0;
	std::vector<greedyListItem*> possibleBrickStates;
};

class GreedyBrick
{
public:
	GreedyBrick(const Image* _img, const std::vector<uint8_t>& colorIDPixels);
	std::vector<greedyListItem> greedyBrick(bool primaryRotation);

	void reset();
private:
	std::vector<greedyListItem*> greedyListItems;
	std::vector<PixelData> pixels;
	const Image* img;
	unsigned int width, height;


	void dump_processed();
	inline bool inBounds(int posX, int posY);
	void collapseBrickState(greedyListItem* item);
	bool testBrickFit(const unsigned int posX, const unsigned int posY, const unsigned int scaleX, const unsigned int scaleY, const uint8_t testColorID);
	void populateAllBrickStates(PixelData& pixel, const bool rotation);
	std::vector<greedyListItem*> getBestBrick();
};

#pragma once
#include "pch.h"

typedef int brickSizes;
typedef unsigned short int brickItemIndex;
typedef vec2<unsigned short int> PixelPos;

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
	bool isValid : 1 = true;
	bool rotation : 2 = 0;
	uint8_t colorID : 6 = 0;
	brickItemIndex brickid;

	PixelPos pos{ 0, 0 };

	greedyListItem(PixelPos _pos, uint8_t _colorID);
};



struct PixelData
{
	PixelPos pos{ 0, 0 };
	uint8_t colorID = undefinedColorID;
	bool rotation = false;

	const static int undefinedColorID = 254;

	std::unordered_set<greedyListItem*> possibleBrickStates;
};

class GreedyBrick
{
public:
	GreedyBrick(const Image* _img, const std::vector<uint8_t>& colorIDPixels);
	std::vector<greedyListItem> greedyBrick();

private:
	std::mutex populateStatesLock;

	std::vector<std::unordered_set<greedyListItem*>> greedyListBricks;
	//remove this entirely?
	std::unordered_set<greedyListItem*> greedyListItems;
	
	std::vector<greedyListItem*> allGreedyItems;
	std::vector<PixelData> pixels;
	const Image* img;
	unsigned int width, height;

	inline bool inBounds(int posX, int posY);
	void collapseBrick(greedyListItem* item, bool fullCollapse = false);
	bool testBrickFit(const unsigned int posX, const unsigned int posY, const unsigned int scaleX, const unsigned int scaleY, const uint8_t testColorID);
	void populateAllBrickStates(PixelData& pixel, const bool rotation);
	std::vector<greedyListItem*> getBestBrick();
};

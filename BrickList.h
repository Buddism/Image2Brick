#pragma once
#include "pch.h"

class BrickList
{
public:
	BrickList();

	brickSizes getBrickSizes(int x, int y);
	brickItemIndex getBrickIndex(int x, int y);

	bool readBrickList();
	std::vector<brickListItem> info;
	std::vector<uint16_t> volumes;
private:
	std::unordered_map<brickSizes, brickItemIndex> lookup;

	void add(int sizeX, int sizeY, std::string uiName);

	void sortByVolume();
};

extern BrickList bricklist;
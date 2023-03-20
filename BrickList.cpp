#include "pch.h"
BrickList bricklist;


BrickList::BrickList()
{
}

brickSizes BrickList::getBrickSizes(int x, int y)
{
	return (x & 0xFFFF) + (y << 16);
}
brickItemIndex BrickList::getBrickIndex(int x, int y)
{
	brickSizes test1 = getBrickSizes(x, y);
	if (lookup.contains(test1))
		return lookup[test1];

	return -1;
}
void BrickList::add(int sizeX, int sizeY, std::string uiName)
{
	brickListItem item{ sizeX, sizeY, uiName };

	lookup[getBrickSizes(sizeX, sizeY)] = (brickItemIndex)info.size();
	lookup[getBrickSizes(sizeY, sizeX)] = (brickItemIndex)info.size();
	item.id = (brickItemIndex)-1;

	info.push_back(item);
}

void BrickList::sortByVolume()
{
	//sorts by sizeX, then sizeY
	auto brickSort = [](brickListItem& a, brickListItem& b)
	{
		if (a.sizeX == b.sizeY)
			return a.sizeY < b.sizeY;

		return a.sizeX < b.sizeX;
	};
	std::sort(info.begin(), info.end(), brickSort);

	for (int i = 0; i < info.size(); i++)
	{
		info[i].id = i;
	}
}
bool BrickList::readBrickList()
{
	std::ifstream myFile;
	std::string filePath = dataspace + "bricklist.txt";
	myFile.open(filePath);

	if (!myFile.is_open())
		return false;

	std::string line;
	while (std::getline(myFile, line))
	{
		std::stringstream ss(line);

		int sizeX, sizeY;
		std::string uiName;
		ss >> sizeX;
		ss >> sizeY;
		ss.ignore(1);
		std::getline(ss, uiName, '\n');


		add(sizeX, sizeY, uiName);

		//std::cout << std::format("sizeX: {}, sizeY: {}, uiName: '{}'\n", sizeX, sizeY, uiName);
	}

	sortByVolume();
	return true;
}

#include "pch.h"
#include <omp.h>
#include <mutex>

template<typename T>
void fast_vector_remove(std::vector<T>& vector, size_t index)
{
	if (vector.size() > 0)
	{
		vector[index] = vector[vector.size() - 1];
		vector.pop_back();
	}
}

greedyListItem::greedyListItem(PixelPos _pos, uint8_t _colorID)
{
	pos = _pos;
	colorID = _colorID;
}

std::string greedyListItem::toString()
{
	return std::format("{}: {} {}", bricklist.info[this->brickid].uiName, this->pos.x, this->pos.y);
}

//PUBLIC METHODS:
GreedyBrick::GreedyBrick(const Image* _img, const std::vector<uint8_t>& colorIDPixels)
{
	img = _img;

	width = img->width;
	height = img->height;

	int maxIndex = width * height;
	pixels = new PixelData[maxIndex];

	for (unsigned short int y = 0; y < height; y++)
	{
		for (unsigned short int x = 0; x < width; x++)
		{
			unsigned int index = x + y * width;
			if (index >= maxIndex)
				return;

			pixels[index].previousLargestBrickID[0] = bricklist.info.size() - 1;
			pixels[index].previousLargestBrickID[1] = bricklist.info.size() - 1;
			pixels[index].colorID = colorIDPixels[index];
			pixels[index].pos = { x, y };
		}
	}
}

std::vector<greedyListItem> GreedyBrick::greedyBrick()
{
	std::cout << "counting all brick states\n";

	std::atomic<uint32_t> numPixels;
	std::mutex coutLock;

	std::atomic<uint64_t> numBrickPotentials = 0;
	std::atomic<uint32_t> numVisiblePixels = 0;
#pragma omp parallel for
	for (int y = 0; y < height; y++)
	{
		for (int x = 0; x < width; x++)
		{
			int index = x + y * width;
			PixelData& pixel = pixels[index];
			numPixels++;
			if (numPixels % 100 == 0)
			{
				coutLock.lock();
				std::cout << std::format("{:1f}%      \r", 100 * (float)numPixels / (width * height));
				coutLock.unlock();
			}

			if (pixel.colorID == PixelData::undefinedColorID)
			{
				std::cout << "PIXEL WAS UNDEFINED?\n";
				DebugBreak();
			}

			if (pixel.colorID >= PixelData::maxColors)
				continue;

			calculateAllBrickStates(pixel, false);
			calculateAllBrickStates(pixel, true);

			numBrickPotentials += pixel.totalPossibleStates;
			numVisiblePixels++;
		}
	}

	uint32_t numPixelsLeft = numVisiblePixels;
	std::cout << "\r100%           \n";
	std::cout << "counted " << numBrickPotentials << " num brick states\n";

	std::cout << "collapsing brick states\n";
	greedyListItem item;
	while (true)
	{
		item = getBestBrick();
		if (item.colorID == PixelData::undefinedColorID)
		{
			//collapse all remaining 1x1f
			for (int i = 0; i < width * height; i++)
			{
				PixelData& pixel = pixels[i];
				if (pixel.colorID >= PixelData::maxColors)
					continue;
				
				item.pos = pixel.pos;
				item.brickid = 0;
				item.colorID = pixel.colorID;
				outputGreedyList.push_back(item);
			}
			break;
		}

		numPixelsLeft -= bricklist.volumes[item.brickid];
		collapseBrick(item, true);

		outputGreedyList.push_back(item);

		std::cout << std::format("{:1f}%      {}                 \r", 100 * (1 - (float)numPixelsLeft / numVisiblePixels), bricklist.info[item.brickid].uiName);
	}

	std::cout << "\r100%           \n";

	return outputGreedyList;
}

//PRIVATE METHODS:
void GreedyBrick::collapseBrick(greedyListItem& item, bool fullCollapse)
{
	brickListItem& brick = bricklist.info[item.brickid];
	const int sizeX = !item.rotation ? brick.sizeX : brick.sizeY;
	const int sizeY = !item.rotation ? brick.sizeY : brick.sizeX;

	const int highX = item.pos.x + sizeX;
	const int highY = item.pos.y + sizeY;

	for (int y = item.pos.y; y < highY; y++)
	{
		for (int x = item.pos.x; x < highX; x++)
		{
			PixelData& pixel = pixels[x + y * width];
			DEBUG_ASSERT(x + y * width > width * height);

			if (!fullCollapse)
			{
				//partial collapse: just remove our current item from everything it overlaps
				pixel.totalPossibleStates--;
				continue;
			}

			//total collapse: remove everything overlapping with our current item including possible brick states
			fullCollapseBrick(pixel, false);
			fullCollapseBrick(pixel, true);
		}
	}

	//update the pixels after for more accurate state tracking
	if (fullCollapse)
	{
		for (int y = item.pos.y; y < highY; y++)
		{
			for (int x = item.pos.x; x < highX; x++)
			{
				pixels[x + y * width].colorID = PixelData::satisifedColorID;
			}
		}
	}
}

void GreedyBrick::fullCollapseBrick(PixelData& pixel, bool rotation)
{
	const unsigned short int posX = pixel.pos.x;
	const unsigned short int posY = pixel.pos.y;
	const int colorID = pixel.colorID;

	//remove the 1x1f
	if(!rotation)
		pixel.totalPossibleStates--;
	
	for (brickItemIndex brickIdx = 1; brickIdx <= pixel.previousLargestBrickID[rotation]; brickIdx++)
	{
		const brickListItem& currBrick = bricklist.info[brickIdx];
		if (rotation && currBrick.sizeX == currBrick.sizeY)
			continue;

		const int sizeX = !rotation ? currBrick.sizeX : currBrick.sizeY;
		const int sizeY = !rotation ? currBrick.sizeY : currBrick.sizeX;

		uint32_t unused;
		if (testBrickFit(posX, posY, sizeX, sizeY, colorID, unused))
		{
			greedyListItem collapseItem;
			collapseItem.rotation = rotation;
			collapseItem.brickid = brickIdx;
			collapseItem.pos = { posX, posY };

			collapseBrick(collapseItem);
		}
	}
}


greedyListItem GreedyBrick::getBestBrick()
{
	greedyListItem bestBrick;
	bestBrick.colorID = PixelData::undefinedColorID;
	bestBrick.brickid = 1;
	bestBrick.totalPossibleStates = UINT32_MAX;

	PixelData* firstSatisfiedPixel = nullptr;
	for(int i = 0; i < width * height; i++)
	{
		PixelData& pixel = pixels[i];
		if (pixel.colorID >= PixelData::maxColors)
		{
			if (firstSatisfiedPixel == nullptr)
				firstSatisfiedPixel = &pixel;

			if (pixel.nextUnsatisfiedPixel > 0)
				i = pixel.nextUnsatisfiedPixel - 1;

			continue;
		}

		if (firstSatisfiedPixel != nullptr)
		{
			firstSatisfiedPixel->nextUnsatisfiedPixel = i;
			firstSatisfiedPixel = nullptr;	
		}

		getBestPossibleBrick(pixel, bestBrick, false);
		getBestPossibleBrick(pixel, bestBrick, true);

		//it can only be a 1x1
		if (pixel.totalPossibleStates == 1)
		{
			greedyListItem item;
			item.pos = pixel.pos;
			item.brickid = 0;
			item.colorID = pixel.colorID;
			outputGreedyList.push_back(item);

			pixel.colorID = PixelData::satisifedColorID;
		}
	}

	return bestBrick;
}

//largest brick with the lowest entropy
void GreedyBrick::getBestPossibleBrick(PixelData& pixel, greedyListItem& bestBrick, const bool rotation)
{
	const unsigned short int posX = pixel.pos.x;
	const unsigned short int posY = pixel.pos.y;
	const int colorID = pixel.colorID;

	brickItemIndex bestMinimalVolumeId = bricklist.info[bestBrick.brickid].minimalVolumeId;

	bool hasFoundLargest = false;
	for (int brickIdx = pixel.previousLargestBrickID[rotation]; brickIdx >= bestMinimalVolumeId; brickIdx--)
	{
		const brickListItem& currBrick = bricklist.info[brickIdx];

		//no longer in the same volume category
		if (hasFoundLargest && currBrick.minimalVolumeId < pixel.previousLargestMinVolumeID[rotation])
			break;

		const int sizeX = !rotation ? currBrick.sizeX : currBrick.sizeY;
		const int sizeY = !rotation ? currBrick.sizeY : currBrick.sizeX;

		if (!hasFoundLargest)
		{
			pixel.previousLargestBrickID[rotation] = brickIdx;
			pixel.previousLargestMinVolumeID[rotation] = currBrick.minimalVolumeId;
		}

		uint32_t totalPossibleStates = 0;
		if (testBrickFit(posX, posY, sizeX, sizeY, colorID, totalPossibleStates))
		{
			//volume > bestVolume || volume == bestVolume && less entropy
			if (currBrick.minimalVolumeId > bestMinimalVolumeId || currBrick.minimalVolumeId == bestMinimalVolumeId && totalPossibleStates < bestBrick.totalPossibleStates)
			{
				bestBrick.rotation = rotation;
				bestBrick.colorID = colorID;
				bestBrick.brickid = brickIdx;
				bestBrick.totalPossibleStates = totalPossibleStates;
				bestBrick.pos = { posX, posY };

				bestMinimalVolumeId = currBrick.minimalVolumeId;
			}

			hasFoundLargest = true;
		}
	}
}

void GreedyBrick::calculateAllBrickStates(PixelData& pixel, const bool rotation)
{
	const unsigned short int posX = pixel.pos.x;
	const unsigned short int posY = pixel.pos.y;
	const int colorID = pixel.colorID;

	//1x1f
	if (!rotation)
		pixel.totalPossibleStates++;
	uint32_t doNothing;
	for (brickItemIndex brickIdx = 1; brickIdx < bricklist.info.size(); brickIdx++)
	{
		const brickListItem& currBrick = bricklist.info[brickIdx];
		if (rotation && currBrick.sizeX == currBrick.sizeY)
			continue;

		const int sizeX = !rotation ? currBrick.sizeX : currBrick.sizeY;
		const int sizeY = !rotation ? currBrick.sizeY : currBrick.sizeX;


		if (testBrickFit(posX, posY, sizeX, sizeY, colorID, doNothing))
		{
			//add our new item to every pixel it overlaps with
			const int highX = posX + sizeX;
			const int highY = posY + sizeY;

			for (int y = posY; y < highY; y++)
			{
				for (int x = posX; x < highX; x++)
				{
					DEBUG_ASSERT(x + y * width > width * height);

					PixelData& overlapPixel = pixels[x + y * width];
					overlapPixel.totalPossibleStates++;
				}
			}
		}
	}
}

bool GreedyBrick::testBrickFit(const unsigned int posX, const unsigned int posY, const unsigned int scaleX, const unsigned int scaleY, const uint8_t testColorID, uint32_t& totalPossibleStates)
{
	const unsigned int highX = posX + scaleX;
	const unsigned int highY = posY + scaleY;

	if (highX > width || highY > height)
		return false;

	totalPossibleStates = 0;
	for (int y = posY; y < highY; y++)
	{
		for (int x = posX; x < highX; x++)
		{
			DEBUG_ASSERT(x + y * width > width * height);

			const PixelData& pixel = pixels[x + y * width];
			if (testColorID != pixel.colorID)
				return false;

			totalPossibleStates += pixel.totalPossibleStates;
		}
	}

	return true;
}
#include "pch.h"

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

//PUBLIC METHODS:
GreedyBrick::GreedyBrick(const Image* _img, const std::vector<uint8_t>& colorIDPixels)
{
	img = _img;

	width = img->width;
	height = img->height;

	int maxIndex = width * height;
	pixels.resize(maxIndex);

	for (unsigned int y = 0; y < height; y++)
	{
		for (unsigned int x = 0; x < width; x++)
		{
			int index = x + y * width;

			pixels[index].colorID = colorIDPixels[index];
			pixels[index].pos = { x, y };
		}
	}
}

std::vector<greedyListItem> GreedyBrick::greedyBrick()
{
	std::cout << "generating all brick states\n";
	for (unsigned int y = 0; y < height; y++)
	{
		for (unsigned int x = 0; x < width; x++)
		{
			int index = x + y * width;
			PixelData& pixel = pixels[index];

			if (pixel.colorID == PixelData::undefinedColorID)
			{
				std::cout << "PIXEL WAS UNDEFINED?\n";
				DebugBreak();
			}

			if (pixel.colorID >= 64)
				continue;

			populateAllBrickStates(pixel, false);
			populateAllBrickStates(pixel, true);
		}
	}
	std::cout << "generated " << allGreedyItems.size() << " num brick states\n";

	std::cout << "collapsing brick states\n";

	std::vector<greedyListItem> data;
	while (!greedyListItems.empty())
	{
		std::vector<greedyListItem*> largestEntropyBucket = getBestBrick();

		while (largestEntropyBucket.size() > 0)
		{
			int index = std::rand() % largestEntropyBucket.size();

			greedyListItem* item = largestEntropyBucket[index];
			if (item->isValid)
			{
				collapseBrick(item, true);
				data.push_back(*item);

				//
				break;
			}

			fast_vector_remove(largestEntropyBucket, index);
		}
	}

	for (greedyListItem* item : allGreedyItems)
		delete item;

	allGreedyItems.clear();
	return data;
}

//PRIVATE METHODS:
void GreedyBrick::collapseBrick(greedyListItem* item, bool fullCollapse)
{
	brickListItem& brick = bricklist.info[item->brickid];
	const int sizeX = !item->rotation ? brick.sizeX : brick.sizeY;
	const int sizeY = !item->rotation ? brick.sizeY : brick.sizeX;

	const int highX = item->pos.x + sizeX;
	const int highY = item->pos.y + sizeY;

	item->isValid = false;

	for (int i = 0; i < greedyListItems.size(); i++)
	{
		if (greedyListItems[i] == item)
		{
			fast_vector_remove(greedyListItems, i);
			break;
		}
	}

	for (int y = item->pos.y; y < highY; y++)
	{
		for (int x = item->pos.x; x < highX; x++)
		{
			PixelData& pixel = pixels[x + y * width];
			DEBUG_ASSERT(x + y * width > width * height);

			if (!fullCollapse)
			{
				//partial collapse: just remove our current item from everything it overlaps
				pixel.possibleBrickStates.erase(item);
				continue;
			}

			//total collapse: remove everything overlapping with our current item including possible brick states
			auto unmodifiedStates = std::unordered_set<greedyListItem*>(pixel.possibleBrickStates);
			for (auto item : unmodifiedStates)
			{
				if (item->isValid)
					collapseBrick(item);
			}

			pixel.possibleBrickStates.clear();
		}
	}
}

std::vector<greedyListItem*> GreedyBrick::getBestBrick()
{
	//get a list of the largest bricks
	std::vector<greedyListItem*> largestBrickBucket(0);
	largestBrickBucket.reserve(1024);

	int largestVolume = 0;
	for (auto* item : greedyListItems)
	{
		brickListItem* brick = &bricklist.info[item->brickid];

		if (brick->volume > largestVolume)
		{
			largestBrickBucket.clear();
			largestVolume = brick->volume;
		}

		if (brick->volume == largestVolume)
			largestBrickBucket.push_back(item);
	}

	//for all largest bricks
	std::vector<greedyListItem*> lowestEntropyBucket(0);
	unsigned long int lowestEntropy = UINT64_MAX;
	for (auto* item : largestBrickBucket)
	{
		brickListItem* brick = &bricklist.info[item->brickid];

		const int sizeX = !item->rotation ? brick->sizeX : brick->sizeY;
		const int sizeY = !item->rotation ? brick->sizeY : brick->sizeX;

		const unsigned int highX = item->pos.x + sizeX;
		const unsigned int highY = item->pos.y + sizeY;

		unsigned long int entropy = 0;
		for (int y = item->pos.y; y < highY; y++)
		{
			for (int x = item->pos.x; x < highX; x++)
			{
				entropy += (&pixels[x + y * width])->possibleBrickStates.size();
			}
		}

		if (entropy < lowestEntropy)
		{
			lowestEntropyBucket.clear();
			lowestEntropy = entropy;
		}

		if (entropy == lowestEntropy)
			lowestEntropyBucket.push_back(item);
	}

	return lowestEntropyBucket;
}

void GreedyBrick::populateAllBrickStates(PixelData& pixel, const bool rotation)
{
	const unsigned int posX = pixel.pos.x;
	const unsigned int posY = pixel.pos.y;
	const int colorID = pixel.colorID;

	for (brickItemIndex brickIdx = 0; brickIdx < bricklist.info.size(); brickIdx++)
	{
		const brickListItem& currBrick = bricklist.info[brickIdx];
		const int sizeX = !rotation ? currBrick.sizeX : currBrick.sizeY;
		const int sizeY = !rotation ? currBrick.sizeY : currBrick.sizeX;

		if (testBrickFit(posX, posY, sizeX, sizeY, colorID))
		{
			//add our new item to every pixel it overlaps with
			greedyListItem* item = new greedyListItem(PixelPos{ posX, posY }, colorID);
			item->brickid = currBrick.id;
			item->rotation = rotation;

			allGreedyItems.push_back(item);
			greedyListItems.push_back(item);

			const int highX = posX + sizeX;
			const int highY = posY + sizeY;

			for (int y = posY; y < highY; y++)
			{
				for (int x = posX; x < highX; x++)
				{
					DEBUG_ASSERT(x + y * width > width * height);

					PixelData& overlapPixel = pixels[x + y * width];
					overlapPixel.possibleBrickStates.insert(item);
				}
			}
		}
	}
}

bool GreedyBrick::testBrickFit(const unsigned int posX, const unsigned int posY, const unsigned int scaleX, const unsigned int scaleY, const uint8_t testColorID)
{
	const unsigned int highX = posX + scaleX;
	const unsigned int highY = posY + scaleY;

	if (highX > width || highY > height)
		return false;

	for (int y = posY; y < highY; y++)
	{
		for (int x = posX; x < highX; x++)
		{
			DEBUG_ASSERT(x + y * width > width * height);

			const PixelData& pixel = pixels[x + y * width];
			if (testColorID != pixel.colorID)
				return false;
		}
	}

	return true;
}


inline bool GreedyBrick::inBounds(int posX, int posY)
{
	return (posX >= 0 && posX <= width)
		&& (posY >= 0 && posY <= height);
}



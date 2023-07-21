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

	for (unsigned short int y = 0; y < height; y++)
	{
		for (unsigned short int x = 0; x < width; x++)
		{
			unsigned int index = x + y * width;

			pixels[index].colorID = colorIDPixels[index];
			pixels[index].pos = { x, y };
		}
	}
}

std::vector<greedyListItem> GreedyBrick::greedyBrick()
{
	std::cout << "generating all brick states\n";
	size_t reserveSize = (size_t)height * width * 6;

	allGreedyItems.reserve(reserveSize);
	greedyListBricks.resize(bricklist.info.size());

	std::atomic<long long> numPixels;
	std::mutex coutLock;

#pragma omp parallel for
	for (int y = 0; y < height; y++)
	{
		for (int x = 0; x < width; x++)
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

			numPixels++;

			if (numPixels % 100 == 0)
			{
				coutLock.lock();
				std::cout << std::format("{:1f}%      \r", 100 * (float)numPixels / (width * height));
				coutLock.unlock();
			}
		}
	}

	for (auto* item : allGreedyItems)
		greedyListItems.insert(item);

	std::cout << "\r100%           \n";
	std::cout << "generated " << allGreedyItems.size() << " num brick states\n";

	std::cout << "collapsing brick states\n";

	std::vector<greedyListItem> data;
	while (!greedyListItems.empty())
	{
		std::vector<greedyListItem*> largestEntropyBucket = getBestBrick();
		if (largestEntropyBucket.size() == 0)
		{
			std::cout << "bad things happening?\n";
			break;
		}

		while (largestEntropyBucket.size() > 0)
		{
			int index = std::rand() % largestEntropyBucket.size();

			greedyListItem* item = largestEntropyBucket[index];
			if (item->isValid)
			{
				collapseBrick(item, true);
				data.push_back(*item);

				std::cout << std::format("{:1f}%      \r", 100 - 100 * (float)greedyListItems.size() / allGreedyItems.size());

				break;
			}

			fast_vector_remove(largestEntropyBucket, index);
		}
	}
	std::cout << "\r100%           \n";

	for (greedyListItem* item : allGreedyItems)
		delete item;

	allGreedyItems.clear();
	greedyListBricks.clear();

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

	greedyListItems.erase(item);
	greedyListBricks[item->brickid].erase(item);

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

	int largestVolume = 0;
	for (int i = bricklist.info.size() - 1; i >= 0; i--)
	{
		auto& list = greedyListBricks[i];

		if (list.size() > 0)
		{
			int volume = bricklist.info[i].volume;
			if (volume < largestVolume)
				break;

			//its a 1x1f just return it
			if (volume == 1)
				return { *(list.begin()) };

			largestVolume = volume;

			largestBrickBucket.reserve(largestBrickBucket.size() + list.size());
			largestBrickBucket.insert(largestBrickBucket.end(), list.begin(), list.end());
		}
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
	const unsigned short int posX = pixel.pos.x;
	const unsigned short int posY = pixel.pos.y;
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

			populateStatesLock.lock();

			greedyListBricks[brickIdx].insert(item);
			allGreedyItems.push_back(item);

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

			populateStatesLock.unlock();
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



#include "pch.h"

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

void GreedyBrick::reset()
{
	for (auto& pixel : pixels)
	{
		pixel.rotation = false;
		pixel.was_processed = false;
	}
}
void GreedyBrick::dump_processed()
{
	for (unsigned int y = 0; y < height; y++)
	{
		for (unsigned int x = 0; x < width; x++)
		{
			int index = x + y * width;
			std::cout << (pixels[index].was_processed ? "+" : ".");
		}
		std::cout << "\n";
	}
}

std::vector<greedyListItem> GreedyBrick::greedyBrick(bool primaryRotation)
{
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

			if (pixel.was_processed || pixel.colorID >= 64)
				continue;

			populateAllBrickStates(pixel,  primaryRotation);
			populateAllBrickStates(pixel, !primaryRotation);
		}
	}

	std::vector<greedyListItem*> toCleanup;
	toCleanup.reserve(greedyListItems.size());

	unsigned int pixelsLeft = height * width;
	while (pixelsLeft)
	{
		std::vector<greedyListItem*> largestEntropyBucket = getBestBrick();

		while (largestEntropyBucket.size() > 0)
		{
			int index = std::rand() % largestEntropyBucket.size();
			greedyListItem* item = largestEntropyBucket[index];

			collapseBrickState(item);

			fast_vector_remove(largestEntropyBucket, index);

			toCleanup.push_back(item);
		}
	}

	for (auto* item : toCleanup)
		delete item;

	toCleanup.clear();

	std::vector<greedyListItem> data;

	return data;
}

//PRIVATE METHODS:

//returns a vector of highest entropy items
void GreedyBrick::collapseBrickState(greedyListItem* item)
{
	brickListItem& brick = bricklist.info[item->brickid];
	const int sizeX = !item->rotation ? brick.sizeX : brick.sizeY;
	const int sizeY = !item->rotation ? brick.sizeY : brick.sizeX;

	const int highX = item->pos.x + sizeX;
	const int highY = item->pos.y + sizeY;

	unsigned long int entropy = 0;
	for (int y = item->pos.x; y < highY; y++)
	{
		for (int x = item->pos.y; x < highX; x++)
		{
			PixelData* pixel = &pixels[x + y * width];

			for (auto* item : pixel->possibleBrickStates)
			{

			}
			//todo
		}
	}
}
std::vector<greedyListItem*> GreedyBrick::getBestBrick()
{
	//get a list of the largest bricks
	std::vector<greedyListItem*> largestBrickBucket(0);
	int largestVolume = 0;
	brickListItem* bestBrick = nullptr;
	for (unsigned int index = 0; index < greedyListItems.size(); index++)
	for(auto* item : greedyListItems)
	{
		brickListItem* brick = &bricklist.info[item->brickid];

		if (brick->volume > largestVolume)
			largestBrickBucket.clear();

		if (brick->volume == largestVolume)
		{
			largestBrickBucket.push_back(item);
			largestVolume = brick->volume;
		}
	}

	//for all largest bricks
	std::vector<greedyListItem*> lowestEntropyBucket(0);
	int lowestEntropy = 0;
	for(auto* item : largestBrickBucket)
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
			lowestEntropyBucket.clear();

		if (entropy == largestVolume)
		{
			lowestEntropyBucket.push_back(item);
			lowestEntropy = entropy;
		}
	}

	return lowestEntropyBucket;
}

void GreedyBrick::populateAllBrickStates(PixelData& pixel, const bool rotation)
{
	const unsigned int posX = pixel.pos.x;
	const unsigned int posY = pixel.pos.y;
	const int colorID = pixel.colorID;

	for(brickItemIndex brickIdx = 0; brickIdx < bricklist.info.size(); brickIdx++)
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

			greedyListItems.push_back(item);

			const int highX = posX + sizeX;
			const int highY = posY + sizeY;

			for (int y = posY; y < highY; y++)
			{
				for (int x = posX; x < highX; x++)
				{
					DEBUG_ASSERT(x + y * width > width * height);

					PixelData* overlapPixel = &pixels[x + y * width];
					overlapPixel->possibleBrickStates.push_back(item);
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
			if (testColorID != pixel.colorID || pixel.was_processed)
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



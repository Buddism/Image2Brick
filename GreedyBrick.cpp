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

	size_t maxIndex = width * height;
	pixels.resize(maxIndex);

	for(int index = 0; index < maxIndex; index++)
		pixels[index].colorID = colorIDPixels[index];
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
	std::vector<greedyListItem> data;
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

			int colorID = pixel.colorID;

			brickListItem& brick1 = bricklist.info[findLargestBrick(x, y, colorID,  primaryRotation)];
			brickListItem& brick2 = bricklist.info[findLargestBrick(x, y, colorID, !primaryRotation)];

			greedyListItem listItem(PixelPos(x, y), colorID);
			brickListItem* largestBrick;
			if (brick1.volume > brick2.volume)
			{
				largestBrick = &brick1;
				listItem.rotation = primaryRotation;
			}
			else {
				largestBrick = &brick2;
				listItem.rotation = !primaryRotation;
			}

			listItem.brickid = largestBrick->id;

			unsigned int sizeX = !listItem.rotation ? largestBrick->sizeX : largestBrick->sizeY;
			unsigned int sizeY = !listItem.rotation ? largestBrick->sizeY : largestBrick->sizeX;

			setBrickFitProccessed(x, y, sizeX, sizeY);

			//dump_processed();

			data.push_back(listItem);
		}
	}

	return data;
}

//PRIVATE METHODS:
//this algo is kinda slow but it doesnt matter that much
brickItemIndex GreedyBrick::findLargestBrick(const unsigned int posX, const unsigned int posY, const int colorID, const bool rotation)
{
	unsigned int best_volume = 0;
	brickItemIndex best_item = 0;
	for(brickItemIndex brickIdx = 1; brickIdx < bricklist.info.size(); brickIdx++)
	{
		const brickListItem& currBrick = bricklist.info[brickIdx];
		const int sizeX = !rotation ? currBrick.sizeX : currBrick.sizeY;
		const int sizeY = !rotation ? currBrick.sizeY : currBrick.sizeX;

		if (currBrick.volume > best_volume && testBrickFit(posX, posY, sizeX, sizeY, colorID))
		{
			best_volume = currBrick.volume;
			best_item = currBrick.id;

			DEBUG_ASSERT(currBrick.id != brickIdx);
		}
	}

	return best_item;
}

bool GreedyBrick::testBrickFit(const unsigned int posX, const unsigned int posY, const unsigned int scaleX, const unsigned int scaleY, const uint8_t testColorID)
{
	const int highX = posX + scaleX;
	const int highY = posY + scaleY;

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

void GreedyBrick::setBrickFitProccessed(const unsigned int posX, const unsigned int posY, const unsigned int scaleX, const unsigned int scaleY)
{
	const int highX = posX + scaleX;
	const int highY = posY + scaleY;

	DEBUG_ASSERT(highX > width || highY > height);

	for (int y = posY; y < highY; y++)
	{
		for (int x = posX; x < highX; x++)
		{
			DEBUG_ASSERT(x + y * width >= width * height);

			pixels[x + y * width].was_processed = true;
		}
	}
}


inline bool GreedyBrick::inBounds(int posX, int posY)
{
	return (posX >= 0 && posX <= width)
		&& (posY >= 0 && posY <= height);
}



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
	int maxIndex = img->width * img->height;
	pixels.resize(maxIndex);

	for(int index = 0; index < maxIndex; index++)
		pixels[index].colorID = colorIDPixels[index];
}

void GreedyBrick::dump_processed()
{
	for (unsigned int y = 0; y < img->height; y++)
	{
		for (unsigned int x = 0; x < img->width; x++)
		{
			int index = x + y * img->width;
			std::cout << (pixels[index].was_processed ? "+" : ".");
		}
		std::cout << "\n";
	}
}

std::vector<greedyListItem> GreedyBrick::greedyBrick()
{
	std::vector<greedyListItem> data;
	for (unsigned int y = 0; y < img->height; y++)
	{
		for (unsigned int x = 0; x < img->width; x++)
		{
			int index = x + y * img->width;
			PixelData& pixel = pixels[index];

			if (pixel.colorID == PixelData::undefinedColorID)
			{
				std::cout << "PIXEL WAS UNDEFINED?\n";
				DebugBreak();
			}

			if (pixel.was_processed || pixel.colorID >= 64)
				continue;

			int colorID = pixel.colorID;

			brickListItem& brick1 = bricklist.info[findLargestBrick(x, y, colorID, false)];
			brickListItem& brick2 = bricklist.info[findLargestBrick(x, y, colorID, true )];

			brickListItem largestBrick;
			greedyListItem listItem(PixelPos(x, y), colorID);
			if (brick1.volume > brick2.volume)
			{
				largestBrick = brick1;
				listItem.brickid = brick1.id;
				listItem.rotation = false;

				setBrickFitProccessed(x, y, brick1.sizeX, brick1.sizeY);
			}
			else {
				largestBrick = brick2;
				listItem.brickid = brick2.id;
				listItem.rotation = true;

				setBrickFitProccessed(x, y, brick2.sizeY, brick2.sizeX);
			}

			//std::cout << std::format("uiname: {}\n", largestBrick.uiName);
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
	int best_volume = 0;
	brickItemIndex best_item = 0;
	for(brickItemIndex brickIdx = 0; brickIdx < bricklist.info.size(); brickIdx++)
	{
		brickListItem currBrick = bricklist.info[brickIdx];
		int sizeX = rotation ? currBrick.sizeY : currBrick.sizeX;
		int sizeY = rotation ? currBrick.sizeX : currBrick.sizeY;

		if (currBrick.volume > best_volume && testBrickFit(posX, posY, sizeX, sizeY, colorID))
		{
			best_volume = currBrick.volume;
			best_item = currBrick.id;
		}
	}

	return best_item;
}

bool GreedyBrick::testBrickFit(const unsigned int posX, const unsigned int posY, const unsigned int scaleX, const unsigned int scaleY, const uint8_t testColorID)
{
	const int highX = posX + scaleX;
	const int highY = posY + scaleY;
	if (highX > img->width || highY > img->height)
		return false;

	for (int y = posY; y < highY; y++)
	{
		for (int x = posX; x < highX; x++)
		{
			DEBUG_ASSERT(x + y * img->width > img->width * img->height);

			const PixelData& pixel = pixels[x + y * img->width];
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

	DEBUG_ASSERT(highX > img->width || highY > img->height);

	for (int y = posY; y < highY; y++)
	{
		for (int x = posX; x < highX; x++)
		{
			DEBUG_ASSERT(x + y * img->width >= img->width * img->height);

			pixels[x + y * img->width].was_processed = true;
		}
	}
}


bool GreedyBrick::inBounds(int posX, int posY)
{
	return (posX >= 0 && posX <= img->width)
		&& (posY >= 0 && posY <= img->height);
}



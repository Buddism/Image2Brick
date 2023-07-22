#include "pch.h"

unsigned getClosestColor(unsigned color)
{
	const int t[3] = { color >> 16, (color >> 8) & 0xFF, color & 0xFF };
	double least_penalty = 1e99;
	unsigned chosen = 0;
	for (unsigned index = 0; index < numPalColors; ++index)
	{
		const unsigned pal_color = pal[index];
		const int pc[3] = { pal_color >> 16, (pal_color >> 8) & 0xFF, pal_color & 0xFF };

		//double penalty = abs(pc[0] - t[0]) + abs(pc[1] - t[1]) + abs(pc[2] - t[2]);
		double penalty = pow(abs(pc[0] - t[0]), 2) + pow(abs(pc[1] - t[1]), 2) + pow(abs(pc[2] - t[2]), 2);
		if (penalty < least_penalty)
		{
			least_penalty = penalty;
			chosen = index;
		}
	}

	return chosen;
}

unsigned int img_closest(Image* img, std::vector<uint8_t>& colorIDPixels)
{
	unsigned width = img->width, height = img->height;

	unsigned int numBricks = 0;

#pragma omp parallel for
	for (int y = 0; y < height; ++y)
		for (int x = 0; x < width; ++x)
		{
			unsigned color = img->getPixelRGBA(x, y);
			if (img->channels == 4)
			{
				unsigned alpha = color >> 24;
				if (alpha < 200)
				{
					colorIDPixels[x + y * width] = PixelData::AlphaColorId;
					continue;
				}
				color &= 0xFFFFFF; //strip off the ALPHA
			}

			unsigned index = getClosestColor(color);

			colorIDPixels[x + y * width] = index;
			numBricks++;
			img->num_visible_pixels++;
		}

	return numBricks;
}
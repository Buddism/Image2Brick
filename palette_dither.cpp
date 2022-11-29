#include "pch.h"

//https://bisqwit.iki.fi/story/howto/dither/jy/

/* 8x8 threshold map (note: the patented pattern dithering algorithm uses 4x4) */
static const unsigned char map[8 * 8] = {
	 0,48,12,60, 3,51,15,63,
	32,16,44,28,35,19,47,31,
	 8,56, 4,52,11,59, 7,55,
	40,24,36,20,43,27,39,23,
	 2,50,14,62, 1,49,13,61,
	34,18,46,30,33,17,45,29,
	10,58, 6,54, 9,57, 5,53,
	42,26,38,22,41,25,37,21
};

/* 4x4 threshold map */
//static const unsigned char map[4 * 4] = {
//	0, 12, 3, 15,
//	8, 4, 11, 7,
//	2, 14, 1, 13,
//	10, 6, 9, 5 
//};

/* Palette */

/* Luminance for each palette entry, to be initialized as soon as the program begins */
static unsigned luma[64];

bool PaletteCompareLuma(unsigned index1, unsigned index2)
{
	return luma[index1] < luma[index2];
}
double ColorCompare(int r1, int g1, int b1, int r2, int g2, int b2)
{
	double luma1 = (r1 * 299 + g1 * 587 + b1 * 114) / (255.0 * 1000);
	double luma2 = (r2 * 299 + g2 * 587 + b2 * 114) / (255.0 * 1000);
	double lumadiff = luma1 - luma2;
	double diffR = (r1 - r2) / 255.0, diffG = (g1 - g2) / 255.0, diffB = (b1 - b2) / 255.0;
	return (diffR * diffR * 0.299 + diffG * diffG * 0.587 + diffB * diffB * 0.114) * 0.75
		+ lumadiff * lumadiff;
}
struct MixingPlan
{
	unsigned colors[64];
};
MixingPlan DeviseBestMixingPlan(unsigned color)
{
	MixingPlan result = { {0} };
	const int src[3] = { color >> 16, (color >> 8) & 0xFF, color & 0xFF };

	const double X = 0.09;  // Error multiplier d:0.09
	int e[3] = { 0, 0, 0 }; // Error accumulator
	for (unsigned c = 0; c < 64; ++c)
	{
		// Current temporary value
		int t[3] = { src[0] + e[0] * X, src[1] + e[1] * X, src[2] + e[2] * X };
		// Clamp it in the allowed RGB range
		if (t[0] < 0) t[0] = 0; else if (t[0] > 255) t[0] = 255;
		if (t[1] < 0) t[1] = 0; else if (t[1] > 255) t[1] = 255;
		if (t[2] < 0) t[2] = 0; else if (t[2] > 255) t[2] = 255;
		// Find the closest pal_col from the palette
		double least_penalty = 1e99;
		unsigned chosen = c % numPalColors;
		for (unsigned index = 0; index < numPalColors; ++index)
		{
			const unsigned color = pal[index];
			const int pc[3] = { color >> 16, (color >> 8) & 0xFF, color & 0xFF };
			double penalty = ColorCompare(pc[0], pc[1], pc[2], t[0], t[1], t[2]);
			if (penalty < least_penalty)
			{
				least_penalty = penalty;
				chosen = index;
			}
		}
		// Add it to candidates and update the error
		result.colors[c] = chosen;
		unsigned color = pal[chosen];
		const int pc[3] = { color >> 16, (color >> 8) & 0xFF, color & 0xFF };
		e[0] += src[0] - pc[0];
		e[1] += src[1] - pc[1];
		e[2] += src[2] - pc[2];
	}
	// Sort the colors according to luminance
	std::sort(result.colors, result.colors + 64, PaletteCompareLuma);
	return result;
}

unsigned getClosestColor(unsigned color)
{
	const int t[3] = { color >> 16, (color >> 8) & 0xFF, color & 0xFF };
	double least_penalty = 1e99;
	unsigned chosen = 0;
	for (unsigned index = 0; index < numPalColors; ++index)
	{
		const unsigned pal_color = pal[index];
		const int pc[3] = { pal_color >> 16, (pal_color >> 8) & 0xFF, pal_color & 0xFF };
		//double penalty = ColorCompare(pc[0], pc[1], pc[2], t[0], t[1], t[2]);
		double penalty = abs(pc[0] - t[0]) + abs(pc[1] - t[1]) + abs(pc[2] - t[2]);
		if (penalty < least_penalty)
		{
			least_penalty = penalty;
			chosen = index;
		}
	}

	return chosen;
}

void img_dither(Image* img, std::string& returnStr)
{
	unsigned w = img->width, h = img->height;

	for (unsigned c = 0; c < numPalColors; ++c)
	{
		unsigned r = pal[c] >> 16,
			g = (pal[c] >> 8) & 0xFF,
			b = pal[c] & 0xFF;

		luma[c] = r * 299 + g * 587 + b * 114;
	}

#pragma omp parallel for
	for (unsigned y = 0; y < h; ++y)
		for (unsigned x = 0; x < w; ++x)
		{
			unsigned color = img->getPixelRGBA(x, y);
			if (img->channels == 4)
			{
				unsigned alpha = color >> 24;
				if (alpha < 255)
				{
					returnStr[x + y * w] = '-';
					continue;
				}
				color &= 0xFFFFFF; //strip off the ALPHA
			}
			unsigned map_value = map[(x & 7) + ((y & 7) << 3)];
			MixingPlan plan = DeviseBestMixingPlan(color);
			
			//unsigned index = getClosestColor(color);
			unsigned index = plan.colors[map_value];
			if (index > exportChars.length())
			{
				std::cout << "ERROR: INDEX > EXPORTCHAR LENGTH\n";
				return;
			}
			char chr = exportChars[index];

			returnStr[x + y * w] = chr;
		}
}
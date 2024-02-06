#include "pch.h"
//https://bisqwit.iki.fi/story/howto/dither/jy/

//#define MAP16x16
#define MAP8x8
//#define MAP4x4
//#define MAP2x2

#ifdef MAP16x16
//  16x16 Bayer Dithering Matrix
static const unsigned char map[16 * 16] = {
	  0, 191,  48, 239,  12, 203,  60, 251,   3, 194,  51, 242,  15, 206,  63, 254,
	127,  64, 175, 112, 139,  76, 187, 124, 130,  67, 178, 115, 142,  79, 190, 127,
	 32, 223,  16, 207,  44, 235,  28, 219,  35, 226,  19, 210,  47, 238,  31, 222,
	159,  96, 143,  80, 171, 108, 155,  92, 162,  99, 146,  83, 174, 111, 158,  95,
	  8, 199,  56, 247,   4, 195,  52, 243,  11, 202,  59, 250,   7, 198,  55, 246,
	135,  72, 183, 120, 131,  68, 179, 116, 138,  75, 186, 123, 134,  71, 182, 119,
	 40, 231,  24, 215,  36, 227,  20, 211,  43, 234,  27, 218,  39, 230,  23, 214,
	167, 104, 151,  88, 163, 100, 147,  84, 170, 107, 154,  91, 166, 103, 150,  87,
	  2, 193,  50, 241,  14, 205,  62, 253,   1, 192,  49, 240,  13, 204,  61, 252,
	129,  66, 177, 114, 141,  78, 189, 126, 128,  65, 176, 113, 140,  77, 188, 125,
	 34, 225,  18, 209,  46, 237,  30, 221,  33, 224,  17, 208,  45, 236,  29, 220,
	161,  98, 145,  82, 173, 110, 157,  94, 160,  97, 144,  81, 172, 109, 156,  93,
	 10, 201,  58, 249,   6, 197,  54, 245,   9, 200,  57, 248,   5, 196,  53, 244,
	137,  74, 185, 122, 133,  70, 181, 118, 136,  73, 184, 121, 132,  69, 180, 117,
	 42, 233,  26, 217,  38, 229,  22, 213,  41, 232,  25, 216,  37, 228,  21, 212,
	169, 106, 153,  90, 165, 102, 149,  86, 168, 105, 152,  89, 164, 101, 148,  85
};
constexpr unsigned int map_size = 16 * 16;
constexpr unsigned int map_scale = 16 - 1;
#endif // MAP16x16

#ifdef MAP8x8
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
constexpr unsigned int map_size = 8 * 8;
constexpr unsigned int map_scale = 8 - 1;
#endif //MAP8x8

#ifdef MAP4x4
/* 4x4 threshold map */
static const unsigned char map[4 * 4] = {
	0, 12, 3, 15,
	8, 4, 11, 7,
	2, 14, 1, 13,
	10, 6, 9, 5
};
constexpr unsigned int map_size = 4 * 4;
constexpr unsigned int map_scale = 4 - 1;
#endif //MAP4x4

#ifdef MAP2x2
/* 4x4 threshold map */
static const unsigned char map[2 * 2] = {
	0, 2,
	3, 1
};
constexpr unsigned int map_size = 4 * 4;
constexpr unsigned int map_scale = 2 - 1;
#endif //MAP2x2

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
	unsigned colors[map_size];
};
MixingPlan DeviseBestMixingPlan(unsigned color)
{
	MixingPlan result = { {0} };
	const int src[3] = { color >> 16, (color >> 8) & 0xFF, color & 0xFF };

	const double X = 0.09;  // Error multiplier d:0.09
	int e[3] = { 0, 0, 0 }; // Error accumulator
	for (unsigned c = 0; c < map_size; ++c)
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
	std::sort(result.colors, result.colors + map_size, PaletteCompareLuma);
	return result;
}


unsigned int img_dither(Image* img, std::vector<uint8_t> &colorIDPixels)
{
	unsigned width = img->width, height = img->height;

	for (unsigned c = 0; c < numPalColors; ++c)
	{
		unsigned r = pal[c] >> 16,
			g = (pal[c] >> 8) & 0xFF,
			b = pal[c] & 0xFF;

		luma[c] = r * 299 + g * 587 + b * 114;
	}

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

			unsigned map_value = map[(x & map_scale) + ((y & map_scale) << 3)];
			MixingPlan plan = DeviseBestMixingPlan(color);

			unsigned index = plan.colors[map_value];

			colorIDPixels[x + y * width] = index;
			numBricks++;
			img->num_visible_pixels++;
		}

	return numBricks;
}
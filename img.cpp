#include "pch.h"

Image::Image()
{
	data = NULL;
	num_visible_pixels = 0;
}

Image::~Image()
{
	stbi_image_free(data);
}


bool Image::open(std::string filePath, int req_channels)
{
	filepath = filePath.c_str();

	data = stbi_load(filepath, &width, &height, &channels, req_channels); //2 is gray+alpha
	request_channels = req_channels;

	if (data != NULL)
	{
		std::cout << "OPENED IMAGE: " << filepath << "\n";
		std::cout << " channels: " << channels << "\n";
	}

	return data != NULL;
}

bool Image::is_open()
{
	return data != NULL;
}

//The return value from an image loader is an 'unsigned char *' which points to the pixel data, or NULL
//The pixel data consists of *height scanlines of *width pixels, with each pixel consisting of *components interleaved 8-bit components
//the first pixel pointed to is top-left-most in the image

uint32_t Image::getPixelRGBA(int x, int y)
{
	if (!is_open())
		return UINT32_MAX;

	uint32_t ret = 0;

	int maxChannels = channels < 3 ? channels : 3;
	for (int i = 0; i < maxChannels; i++)
	{
		ret <<= 8; //shift 8 bits over
		uint8_t byte = data[(x + y * width) * channels + i];
		//std::cout << i << " byte: " << unsigned(byte) << "\n";
		ret |= byte;
	}

	if (channels == 4)
		ret |= data[(x + y * width) * 4 + 3] << 24;

	return ret;
}

uint32_t Image::getPixelRGB(int x, int y)
{
	if (!is_open())
		return UINT32_MAX;

	uint32_t ret = 0;
	int maxChannels = channels < 3 ? channels : 3;
	for (int i = 0; i < maxChannels; i++)
	{
		ret <<= 8; //shift 8 bits over
		uint8_t byte = data[(x + y * width) * channels + i];
		//std::cout << i << " byte: " << unsigned(byte) << "\n";
		ret |= byte;
	}

	return ret;
}
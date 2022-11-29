#pragma once
#include "pch.h"

class Image
{
public:
	Image();
	~Image();

	bool is_open();
	bool open(std::string filePath, int req_channels = 0);
	uint32_t getPixelRGBA(int x, int y);
	uint32_t getPixelRGB(int x, int y);

	unsigned char* data;
	const char* filepath;
	int width, height, channels, request_channels;
};


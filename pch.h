#pragma once

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <iostream>
#include <algorithm>
#include <span>
#include <array>

#include <thread>
#include <chrono>
#include <cstdint>
#include <filesystem>

#include <windows.h>
#include <stdint.h>

#include "img.h"
#include "pixel.h"

#define STB_IMAGE_STATIC
#define __STDC_LIB_EXT1__
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION

#include "stb_image.h"
#include "stb_image_write.h"

unsigned int img_dither(Image* img, std::string& returnStr);

extern std::string dataspace;
extern std::string exportChars;
extern unsigned pal[64];
extern int numPalColors;


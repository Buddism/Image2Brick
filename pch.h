#pragma once

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <iostream>
#include <algorithm>
#include <span>
#include <array>
#include <unordered_map>
#include <queue>

#include <sstream>
#include <thread>
#include <chrono>
#include <cstdint>
#include <filesystem>

#include <windows.h>
#include <stdint.h>
#include <stdlib.h>

#include "img.h"
#include "pixel.h"
#include "vec2.h"
#include "GreedyBrick.h"
#include "BrickList.h"

#define STB_IMAGE_STATIC
#define __STDC_LIB_EXT1__
#define STB_IMAGE_IMPLEMENTATION

#include "stb_image.h";

bool readBrickList();
unsigned int img_dither(Image* img, std::vector<uint8_t>& colorIDPixels);

extern std::string dataspace;
extern std::string exportChars;
extern unsigned pal[64];
extern int numPalColors;

#ifdef _DEBUG
#define DEBUG_IF(x) if(x)
#define DEBUG_ASSERT(check) DEBUG_IF(check) DebugBreak();
#else
#define DEBUG_IF(x) if(false)
#define DEBUG_ASSERT(check)
#endif

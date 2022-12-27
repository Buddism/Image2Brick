#include "pch.h"

#ifdef _DEBUG
constexpr int WAIT_TO_CLOSE_TIME = 10;
#else
constexpr int WAIT_TO_CLOSE_TIME = 1;
#endif

void setClipboard(const char* textToSave)
{
	if (textToSave == nullptr)
		return;

	const size_t len = strlen(textToSave) + 1;
	HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE, len);
	memcpy(GlobalLock(hMem), textToSave, len);
	GlobalUnlock(hMem);
	OpenClipboard(0);
	EmptyClipboard();
	SetClipboardData(CF_TEXT, hMem);
	CloseClipboard();
}

bool readColorset()
{
	std::ifstream myFile;
	std::string filePath = dataspace + "colorset.txt";
	myFile.open(filePath);

	if (!myFile.is_open())
		return false;

	std::string line;
	numPalColors = 0;
	for (int i = 0; i < 64; i++)
	{
		if (!myFile)
			break;

		std::getline(myFile, line);
		pal[numPalColors] = std::atoi(line.c_str());
		numPalColors++;
	}

	return true;
}

void close_wait(int wait = 5)
{
	std::cout << std::format("WAITING FOR {} SECONDS\n", wait);
	std::this_thread::sleep_for(std::chrono::seconds(wait));
}

std::string colorid_to_string(Image* img, std::vector<uint8_t> &colorIDPixels)
{
	std::string headerData = std::format("width {}\nheight {}\nDATA:\n", img->width, img->height);
	std::string dataStr; 

	dataStr.resize(img->width * img->height);

#pragma omp parallel for
	for (int y = 0; y < img->height; y++)
	{
		for (int x = 0; x < img->width; x++)
		{
			uint8_t colorID = colorIDPixels[x + y * img->width];
			char color_chr = colorID < exportChars.length() ? exportChars[colorID] : 255;
			dataStr[x + y * img->width] = color_chr;
		}
	}

	dataStr.insert(0, headerData);
	return dataStr;
}

int main(int argc, char* argv[])
{
	//generate char table
	for(unsigned int i = 1; i < 255; i++) //last char is reserved for alpha
		exportChars += unsigned char(i);

	std::cout << "Have " << argc << " arguments:" << std::endl;
	for (int i = 0; i < argc; ++i) {
		std::cout << argv[i] << std::endl;
	}
	std::cout << "end arguments" << std::endl;

	std::string pngFilePath = dataspace + "input.png";
	if (argc == 2)
	{
		bool exists = std::filesystem::exists(argv[1]);
		if (exists)
			pngFilePath = argv[1];
	}

	Image* img = new Image();
	std::unique_ptr<Image*> imgPtr = std::make_unique<Image*>(img);

	if (!img->open(pngFilePath))
	{
		std::cout << "ERR: '" << pngFilePath << "' NOT OPENED!" << "\n";
		close_wait();
	}
	if (!readColorset())
	{
		std::cout << "ERR: colorset.txt NOT FOUND!" << "\n";
		close_wait();
		return 1;
	}
	if (!bricklist.readBrickList())
	{
		std::cout << "ERR: bricklist.txt NOT FOUND!" << "\n";
		close_wait();
		return 2;
	}

	if (numPalColors == 0)
	{
		std::cout << "ERR: numPalColors is ZERO!" << "\n";
		close_wait();
		return 1;
	}
	
	std::vector<uint8_t> colorIDPixels;
	colorIDPixels.resize(img->width * img->height);

	unsigned int numBricks = img_dither(img, colorIDPixels);
	std::cout << "IMG HAS " << numBricks << " NUM BRICKS\n";

	GreedyBrick greedy (img, colorIDPixels);
	std::vector<greedyListItem> greedyList = greedy.greedyBrick();

	std::cout << std::format("greedyList num items: {}\n", greedyList.size());
	for (auto& item : greedyList)
	{
		brickItemIndex brickID = item.brickid;
		brickListItem& brick = bricklist.info[brickID];

		std::cout << std::format("sizeX:{} sizeY:{} posX:{} posY:{}, uiName: ({}), rot: {}\n", brick.sizeX, brick.sizeY, item.pos.x, item.pos.y, brick.uiName, item.rotation);
	}

	std::string lazy_to_brick;
	int calculated_volume = 0;
	for (auto& item : greedyList)
	{
		brickItemIndex brickID = item.brickid;
		brickListItem& brick = bricklist.info[brickID];
		calculated_volume += brick.volume;

		vec2 pos = item.pos;
		vec2<float> center = { 0, 0 };
		if (!item.rotation)
		{
			center.x = pos.x + brick.sizeX / 2.0f;
			center.y = pos.y + brick.sizeY / 2.0f;
		}
		else {
			center.x = pos.x + brick.sizeY / 2.0f;
			center.y = pos.y + brick.sizeX / 2.0f;
		}

		lazy_to_brick += std::format("{} {} {} {} {}\n", center.x, center.y, item.colorID, item.rotation ? 1 : 0, brick.uiName);
	}

	int expected_Volume = img->width * img->height;
	std::cout << std::format("expected_volume: {}, calculated_volume: {}, DIFF: {}\n", expected_Volume, calculated_volume, calculated_volume - expected_Volume);
	float optimization_percent = ((float)greedyList.size() / (float)expected_Volume) * 100.0f;
	std::cout << std::format("num_pixels: {}, num_bricks: {}, % of original: {:.2f}%\n", expected_Volume, greedyList.size(), optimization_percent);

	std::cout << "FINISHED\n";
	setClipboard(lazy_to_brick.c_str());

	//std::string dataString = colorid_to_string(img, colorIDPixels);
	//setClipboard(dataString.c_str());

	close_wait(WAIT_TO_CLOSE_TIME);
}

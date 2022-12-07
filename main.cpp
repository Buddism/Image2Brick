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
	std::string headerData = std::format("{} {}\n", img->width, img->height);
	std::string dataStr; 

	int wideWidth = img->width + 1;
	dataStr.resize(wideWidth * img->height);

#pragma omp parallel for
	for (int y = 0; y < img->height; y++)
	{
		for (int x = 0; x < img->width; x++)
		{
			uint8_t colorID = colorIDPixels[x + y * img->width];
			char color_chr = colorID < exportChars.length() ? exportChars[colorID] : '-';
			dataStr[x + y * wideWidth] = color_chr;
		}
	}

	//insert \n into the last char of each row
	for (int y = 0; y < img->height; y++)
		dataStr[img->width + y * wideWidth] = '\n';

	dataStr.insert(0, headerData);
	return dataStr;
}

int main(int argc, char* argv[])
{
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

	if (!img->open(pngFilePath))
	{
		std::cout << "ERR: '" << pngFilePath << "' NOT OPENED!" << "\n";
		close_wait();
	}
	if (!readColorset())
	{
		std::cout << "ERR: colorset.txt NOT FOUND!" << "\n";
		close_wait();
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


	std::cout << "FINISHED\n";
	std::string dataString = colorid_to_string(img, colorIDPixels);
	setClipboard(dataString.c_str());

	delete img;

	close_wait(WAIT_TO_CLOSE_TIME);
}

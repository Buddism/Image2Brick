#include "pch.h"

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
	std::this_thread::sleep_for(std::chrono::seconds(wait));
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

	std::stringstream ss;
	ss << img->width << " " << img->height << "\n";
	std::string detailStr = ss.str();

	std::string retStr;
	retStr.resize(img->width * img->height);
	img_dither(img, retStr);

	retStr.insert(0, detailStr);

	std::cout << "FINISHED\n";

	setClipboard(retStr.c_str());

	delete img;

	close_wait(1);
}

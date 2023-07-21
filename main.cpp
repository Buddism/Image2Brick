#include "pch.h"

#ifdef _DEBUG
constexpr int WAIT_TO_CLOSE_TIME = 10;
#else
constexpr int WAIT_TO_CLOSE_TIME = 10;
#endif

std::string dataspace;

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

int main(int argc, char* argv[])
{
	dataspace = std::filesystem::current_path().string() + '\\';
	std::cout << "current working directory: " << dataspace << "\n";

	IFDEBUG()
	{
		std::cout << "Have " << argc << " arguments:" << std::endl;
		for (int i = 0; i < argc; ++i) {
			std::cout << argv[i] << std::endl;
		}
		std::cout << "end arguments" << std::endl;
	}

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

	char use_closest_chr;
	std::cout << "Enter anything for closest\n";
	IFDEBUG(true)
		use_closest_chr = 1;
	else
		std::cin.get(use_closest_chr);

	int numBricks = 0;
	if (use_closest_chr == 10) //'enter' character
	{
		std::cout << "USING IMAGE DITHER\n";
		numBricks = img_dither(img, colorIDPixels);
	}
	else {
		std::cout << "USING IMAGE CLOSEST\n";
		numBricks = img_closest(img, colorIDPixels);
	}

	std::cout << "IMG HAS " << numBricks << " NUM BRICKS\n";

	GreedyBrick greedy(img, colorIDPixels);
	std::vector<greedyListItem> greedyList = greedy.greedyBrick();

	std::cout << std::format("greedyList num items: {}\n", greedyList.size());

	auto sortFN = [](const greedyListItem i, const greedyListItem j) {
		int ivolume = bricklist.info[i.brickid].volume;
		int jvolume = bricklist.info[j.brickid].volume;

		if (ivolume == jvolume)
		{
			//third sort
			if (i.colorID == j.colorID)
				return i.rotation > j.rotation;

			//secondary sort
			return i.colorID > j.colorID;
		}
		
		//primary sort
		return ivolume < jvolume;
	};

	std::sort(greedyList.begin(), greedyList.end(), sortFN);

	std::vector<bool> was_used;
	was_used.resize(bricklist.info.size(), 0);
	for (const auto& item : greedyList)
	{
		const brickListItem& brick = bricklist.info[item.brickid];

		was_used[brick.id] = true;
	}
	
	std::vector<int> uiNameIDtoIndex;
	uiNameIDtoIndex.resize(bricklist.info.size(), -1);

	std::string lazy_to_brick;
	int uiNameIndex = 0;
	for(int i = 0; i < was_used.size(); i++)
	{
		bool used = was_used[i];
		if (!used)
			continue;

		lazy_to_brick += std::format("{} {}\n", uiNameIndex, bricklist.info[i].uiName);
		uiNameIDtoIndex[i] = uiNameIndex;

		uiNameIndex++;
	}
	lazy_to_brick += "\n";

	int calculated_volume = 0;

	for (const auto& item : greedyList)
	{
		const brickListItem& brick = bricklist.info[item.brickid];
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

		center.y = img->height - center.y;

		int uiNameID = uiNameIDtoIndex[brick.id];
		lazy_to_brick += std::format("{} {} {} {} {}\n", center.x, center.y, item.colorID, item.rotation ? 1 : 0, uiNameID);
	}

	int expected_Volume = img->num_visible_pixels;
	std::cout << std::format("expected_volume: {}, calculated_volume: {}, DIFF: {}\n", expected_Volume, calculated_volume, calculated_volume - expected_Volume);
	float optimization_percent = ((float)greedyList.size() / (float)expected_Volume) * 100.0f;
	std::cout << std::format("num_pixels: {}, num_bricks: {}, % of original: {:.2f}%\n", expected_Volume, greedyList.size(), optimization_percent);

	std::cout << "FINISHED\n";
	setClipboard(lazy_to_brick.c_str());

	close_wait(WAIT_TO_CLOSE_TIME);
}

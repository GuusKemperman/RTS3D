#include "precomp.h"
#include "Surface.h"

// We don't need to hear stb_image's warnings
#ifdef PLATFORM_LINUX
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wmisleading-indentation"
#elif PLATFORM_WINDOWS
#pragma warning(push, 0)        
#endif

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#ifdef PLATFORM_LINUX
#pragma GCC diagnostic pop
#elif PLATFORM_WINDOWS
#pragma pop
#endif // PLATFORM_LINUX

Framework::Surface::Surface(const std::string& filePath) 
{
	std::string filePathCopy = filePath;
	StringFunctions::RemoveLinuxLineEndings(filePathCopy);

	int n;
	int width{};
	int height{};
	unsigned char* data = stbi_load(filePathCopy.c_str(), &width, &height, &n, 4);

	if (data == nullptr)
	{
		LOGERROR("Failed to load image " << filePath);
		assert(false);
	}

	mWidth = static_cast<uint>(width);
	mHeight = static_cast<uint>(height);
	assert(n == 4);

	mPixels = static_cast<Pixel*>(malloc(mWidth * mHeight * sizeof(Pixel)));
	assert(mPixels != nullptr);

	for (size_t i = 0; i < mWidth * mHeight; i++)
	{
		// RGBA
		const uint r = (data[i * n + 0] << 16);
		const uint g = (data[i * n + 1] << 8);
		const uint b = (data[i * n + 2] << 0);
		const uint a = (data[i * n + 3] << 24);

		mPixels[i] = r + g + b + a;
	}
	
	stbi_image_free(data);
}

Framework::Surface::~Surface()
{
	free(mPixels);
}

void Framework::Surface::operator=(const Surface& other)
{
	mWidth = other.mWidth;
	mHeight = other.mHeight;

	size_t numOfBytes = mWidth * mHeight * sizeof(Pixel);

	free(mPixels);
	mPixels = static_cast<Pixel*>(malloc(numOfBytes));
	assert(mPixels != nullptr);

	memcpy(mPixels, other.mPixels, numOfBytes);
}

void Framework::Surface::Resize(const uint sizeX, const uint sizeY)
{
	Pixel* newPixels = static_cast<Pixel*>(malloc(sizeX * sizeY * sizeof(Pixel)));
	assert(newPixels != nullptr);

	constexpr uint shift = 20;
	const uint dx = static_cast<uint>(static_cast<float>(mWidth) / static_cast<float>(sizeX) * powf(2, shift));
	const uint dy = static_cast<uint>(static_cast<float>(mHeight) / static_cast<float>(sizeY) * powf(2, shift));

	for (uint y = 0; y < sizeY; y++)
	{
		for (uint x = 0; x < sizeX; x++)
		{
			uint readxpos = ((dx * x) >> shift) % mWidth;
			uint readypos = ((dy * y) >> shift) % mHeight;
			
			assert(readxpos >= 0
				&& readxpos < mWidth
				&& readypos >= 0
				&& readypos < mHeight);

			newPixels[x + y * sizeX] = mPixels[readxpos + readypos * mWidth];
		}
	}

	mWidth = sizeX;
	mHeight = sizeY;

	free(mPixels);
	mPixels = newPixels;
}
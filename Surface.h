#pragma once

namespace Framework
{
	class Surface
	{
	public:
		Surface() = default;
		Surface(const std::string& filePath);
		~Surface();

		inline Surface(const Surface& other) { *this = other; }
		void operator=(const Surface& other);

		void Resize(const uint sizeX, const uint sizeY);

		using Pixel = uint;
		Pixel* mPixels{};
		uint mWidth{};
		uint mHeight{};
	};
}
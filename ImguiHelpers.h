#pragma once

namespace Framework
{
	class ImGuiFontWrapper;

	class ImguiHelpers
	{
	public:
		static inline constexpr ImVec2 Centre(const ImVec2& windowSize, const ImVec2& elementSize)
		{
			return ImVec2{ Centre(windowSize.x, elementSize.x), Centre(windowSize.y, elementSize.y) };
		}
		static inline constexpr float Centre(const float windowSize, const float elementSize)
		{
			return (windowSize - elementSize) * 0.5f;	
		}

		static constexpr float sDefaultFontSize = 20.0f;

		// Sets the font size, will be the same size in pixels regardless of the font quality. If no font is provided, will use the default font.
		static void SetWindowFontSize(const float size = sDefaultFontSize, const ImGuiFontWrapper* forFont = nullptr);
	};
}
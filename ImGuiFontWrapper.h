#pragma once

namespace Framework
{
	class ImGuiFontWrapper
	{
	public:
		ImGuiFontWrapper(const std::string& filePathAndSize);
		~ImGuiFontWrapper();

		void SendToImguiFontAtlas();

		// Sets the font's scale regardless of the size in the atlas.
		void SetWindowFontSize(const float size) const;

		inline ImFont* GetFont() { assert(mFont != nullptr); return mFont; }
		static inline ImGuiFontWrapper* GetDefaultFont() { assert(sDefaultFont != nullptr); return sDefaultFont; }

	private:
		std::string mFilePath{};
		float mSizeWithFullQuality{};
		float mSizeInAtlas{};
		float mQuality{};
		ImFont* mFont{};

		inline static ImGuiFontWrapper* sDefaultFont{};
	};
}
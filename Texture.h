#pragma once

namespace Framework
{
	namespace Data
	{
		class Scope;
	}
	class Surface;

	class Texture
	{
	public:
		// Paths followed by a type seperated by a comma, e.g. assets/texture.png,imgui. This allows you to store and find textures from the same filepath but different types
		Texture(const std::string& filePathAndType);
		~Texture();

		void SyncSurfaceAndTexture() const;

		inline Surface* const GetOriginalSurface() const { return mOriginalSurface.get(); }
		inline Surface* const GetSurface() const { return mLowerQualitySurface != nullptr ? mLowerQualitySurface.get() : mOriginalSurface.get(); }
		inline GLuint GetId() const { return mId; }

		void OnSettingsChange(const Data::Scope& previousSettings, const Data::Scope& currentSettings);

		constexpr static const char* sDefaultType = "DEFAULT_TYPE";
		constexpr static const char* sImguiType = "IMGUI_TYPE";

		static int GetMaxTextureSizeForDevice();
		static int GetLargestTextureLoadedInSize();

	private:
		void Load(uint maxSize);

		std::string mFilePath{};
		std::string mType{};
		std::unique_ptr<Surface> mOriginalSurface{};
		std::unique_ptr<Surface> mLowerQualitySurface{};

		GLuint mId{};
	};
}
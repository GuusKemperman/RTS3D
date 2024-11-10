#pragma once

namespace Framework
{
	namespace Data
	{
		class Scope;
	}

	class Texture;

	class Sprite
	{
	public:
		Sprite(const std::string& filePath);
		~Sprite();

		inline uint GetNumOfFrames() const { return static_cast<uint>(mFrameStarts.size()); }
		inline glm::ivec2 GetFrameSizePixels() const { return mFrameSizePixels; }
		
		inline void SetFrame(const uint frame) { assert(frame >= 0 && frame < GetNumOfFrames());  mCurrentFrame = frame; }

		void DrawImGui() const;

	private:
		void UpdateFrameStarts();

		std::shared_ptr<Texture> mTexture{};
		
		uint mCurrentFrame{};

		// In UV coördinates
		std::vector<glm::vec2> mFrameStarts{};
		glm::ivec2 mGridSize{};
		uint mNumOfFrames{};

		// In UV coördinates
		glm::vec2 mFrameSizeUV{};
		glm::ivec2 mFrameSizePixels{};
	};
}
#include "precomp.h"
#include "Sprite.h"

#include "Texture.h"
#include "SavedData.h"
#include "Variable.h"
#include "AssetManager.h"
#include "Surface.h"
#include "Scope.h"
#include "Settings.h"

Framework::Sprite::Sprite(const std::string& filePath)
{
	std::string dataString = filePath.substr(sDataRoot.size());
	Data::SavedData spriteData = { dataString };

	std::string texturePath;
	spriteData.GetVariable("texture") >> texturePath;
	mTexture = AssetManager::Inst().GetAsset<Texture>(texturePath + "," + Texture::sImguiType);

	spriteData.GetVariable("gridSize") >> mGridSize;
	spriteData.GetVariable("totalNumOfFrames") >> mNumOfFrames;
	
	UpdateFrameStarts();
}

Framework::Sprite::~Sprite() = default;

void Framework::Sprite::DrawImGui() const
{
	ImGui::Image(reinterpret_cast<void*>((intptr_t)mTexture->GetId()), 
		Math::ToIMGui(mFrameSizePixels), 
		Math::ToIMGui(mFrameStarts[mCurrentFrame]), 
		Math::ToIMGui(mFrameStarts[mCurrentFrame] + mFrameSizeUV));
}

void Framework::Sprite::UpdateFrameStarts()
{
	const Surface* surface = mTexture->GetOriginalSurface();

	mFrameSizeUV = glm::vec2{ 1.0f } / static_cast<glm::vec2>(mGridSize);
	mFrameSizePixels = glm::ivec2{ static_cast<int>(surface->mWidth), static_cast<int>(surface->mHeight) } / mGridSize;

	mFrameStarts.resize(mNumOfFrames);
	for (int y = 0, frameNum = 0; y < mGridSize.y && frameNum < static_cast<int>(mNumOfFrames); y++)
	{
		for (int x = 0; x < mGridSize.x && frameNum < static_cast<int>(mNumOfFrames); x++, frameNum++)
		{
			mFrameStarts[frameNum] = glm::vec2{ static_cast<float>(x), static_cast<float>(y) } * mFrameSizeUV;
		}
	}
}

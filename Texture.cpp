#include "precomp.h"
#include "Texture.h"

#include "Surface.h"
#include "Settings.h"
#include "Scope.h"

#define GL_BGRA 0x80E1

static int sLargestTextureLoadedInSize = 0;

Framework::Texture::Texture(const std::string& filePathAndType)
{
	glGenTextures(1, &mId);

	std::vector<std::string> pathAndType = StringFunctions::SplitString(filePathAndType, ",");
	assert(pathAndType.size() == 1
		|| pathAndType.size() == 2);

	mFilePath = pathAndType[0];
	mType = pathAndType.size() == 2 ? pathAndType[1].c_str() : sDefaultType;
	mOriginalSurface = std::make_unique<Surface>(mFilePath);

	sLargestTextureLoadedInSize = std::max({ static_cast<int>(mOriginalSurface->mWidth), static_cast<int>(mOriginalSurface->mHeight), sLargestTextureLoadedInSize });

	Settings::Inst().mOnSettingsChanged.bind(this, &Texture::OnSettingsChange);

	uint maxSize;
	Settings::Inst().GetSettings().GetVariable("maxTextureSize") >> maxSize;
	Load(maxSize);
}

Framework::Texture::~Texture()
{
	glDeleteTextures(1, &mId);
	CheckGL();

	Settings::Inst().mOnSettingsChanged.unbind(this, &Texture::OnSettingsChange);
}

void Framework::Texture::SyncSurfaceAndTexture() const
{
	const Surface* usingSurface = GetSurface();

	glBindTexture(GL_TEXTURE_2D, mId);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, usingSurface->mWidth, usingSurface->mHeight, 0, GL_BGRA, GL_UNSIGNED_BYTE, usingSurface->mPixels);

	if (mType == sDefaultType)
	{
		glGenerateMipmap(GL_TEXTURE_2D);
	}
	else if (mType == sImguiType)
	{
		glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
	}
	else
	{
		assert(false);
	}

	glBindTexture(GL_TEXTURE_2D, 0);
	CheckGL();
}

void Framework::Texture::OnSettingsChange(const Data::Scope& previousSettings, const Data::Scope& currentSettings)
{
	const Data::Variable& previousValue = previousSettings.GetVariable("maxTextureSize");
	const Data::Variable& currentValue = currentSettings.GetVariable("maxTextureSize");

	if (previousValue == currentValue)
	{
		return;
	}

	uint maxSize;
	currentValue >> maxSize;

	// Don't bother reducing the size if we're already conforming to it.
	if ((mOriginalSurface->mWidth > maxSize 
		|| mOriginalSurface->mHeight > maxSize)
		|| mLowerQualitySurface != nullptr)
	{
		Load(maxSize);
	}
}

int Framework::Texture::GetMaxTextureSizeForDevice()
{
	static int maxSize = -1;
	if (maxSize == -1)
	{
		glGetIntegerv(GL_MAX_TEXTURE_SIZE, &maxSize);
	}
	return maxSize;
}

int Framework::Texture::GetLargestTextureLoadedInSize()
{
	return sLargestTextureLoadedInSize;
}

void Framework::Texture::Load(uint maxSize)
{
	assert(mOriginalSurface != nullptr);

	mLowerQualitySurface.reset();
	if (mOriginalSurface->mWidth > maxSize
		&& mType != sImguiType)
	{
		mLowerQualitySurface = std::make_unique<Surface>(*mOriginalSurface);
		assert(mLowerQualitySurface->mWidth == mLowerQualitySurface->mHeight);
		mLowerQualitySurface->Resize(maxSize, maxSize);
	}

	glBindTexture(GL_TEXTURE_2D, mId);

	if (mType == sDefaultType)
	{
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	}
	else if (mType == sImguiType)
	{
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE); // This is required on WebGL for non power-of-two textures
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE); // Same
	}
	else
	{
		assert(false);
	}

	glBindTexture(GL_TEXTURE_2D, 0);
	SyncSurfaceAndTexture();
}
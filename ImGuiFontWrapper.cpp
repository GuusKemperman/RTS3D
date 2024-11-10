#include "precomp.h"
#include "ImGuiFontWrapper.h"

#include "Settings.h"
#include "Scope.h"
#include "ImguiHelpers.h"

Framework::ImGuiFontWrapper::ImGuiFontWrapper(const std::string& filePathAndSize)
{
	std::vector<std::string> pathAndSize = StringFunctions::SplitString(filePathAndSize, ",");
	assert(pathAndSize.size() == 1
		|| pathAndSize.size() == 2);

	mFilePath = pathAndSize[0];
	mSizeWithFullQuality = pathAndSize.size() == 1 ? 20.0f : std::stof(pathAndSize[1]);
}

Framework::ImGuiFontWrapper::~ImGuiFontWrapper() = default;

void Framework::ImGuiFontWrapper::SendToImguiFontAtlas()
{
	Framework::Settings::Inst().GetSettings().GetVariable("fontQuality") >> mQuality;
	mSizeInAtlas = mSizeWithFullQuality * mQuality;
	mFont = ImGui::GetIO().Fonts->AddFontFromFileTTF(mFilePath.c_str(), mSizeInAtlas);
	mFont->Scale = Framework::ImguiHelpers::sDefaultFontSize / mSizeInAtlas;

	if (mFont == ImGui::GetIO().Fonts->Fonts[0])
	{
		sDefaultFont = this;
	}
}

void Framework::ImGuiFontWrapper::SetWindowFontSize(const float size) const
{
	ImGui::SetWindowFontScale(size / Framework::ImguiHelpers::sDefaultFontSize);
}
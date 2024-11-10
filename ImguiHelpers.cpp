#include "precomp.h"
#include "ImguiHelpers.h"

#include "ImGuiFontWrapper.h"

void Framework::ImguiHelpers::SetWindowFontSize(const float size, const ImGuiFontWrapper* forFont)
{
	if (forFont == nullptr)
	{
		forFont = ImGuiFontWrapper::GetDefaultFont();
	}
	forFont->SetWindowFontSize(size);
}
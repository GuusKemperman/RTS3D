#include "precomp.h"
#include "Settings.h"

#include "SavedData.h"

Framework::Settings::Settings()
{
    mSavedSettings = std::make_unique<Framework::Data::SavedData>("settings.txt");
}

Framework::Settings::~Settings() = default;

const Framework::Data::Scope& Framework::Settings::GetSettings()
{
    return mSavedSettings->GetScope();
}

void Framework::Settings::SetSettings(const Data::Scope& newSettings)
{
    const Data::Scope oldSettings = mSavedSettings->GetScope();
    mSavedSettings->GetScope() = newSettings;
    mOnSettingsChanged(oldSettings, newSettings);
    mSavedSettings->Save();
}

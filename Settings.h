#pragma once
#include "Singleton.h"

#include "Delegate.h"

namespace Framework
{
	namespace Data
	{
		class Scope;
		class SavedData;
	}

	class Settings :
		public Singleton<Settings>
	{
	private:
		friend Singleton;
		Settings();
		~Settings();

	public:
		const Data::Scope& GetSettings() ;
		void SetSettings(const Data::Scope& newSettings);

		// First argument are the previousSettings settings, second argument are the current settings
		sdel::Delegate<void(const Data::Scope&, const Data::Scope&)> mOnSettingsChanged{};

	private:
		std::unique_ptr<Data::SavedData> mSavedSettings{};
	};
}
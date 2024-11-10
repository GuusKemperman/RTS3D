#pragma once
#include "SceneLoader.h"

namespace Framework
{
	namespace Data
	{
		class SavedData;
		class Scope;
	}

	class Scene;

	class Game
	{
	public:
		Game();
		~Game();

		// game flow methods
		void Init();

		// Called just before all the new frame functions.
		void EarlyTick();
		bool Tick(float deltaTime);
		void Shutdown();

		void RequestLoadTo(std::unique_ptr<Scene> scene);

		inline void Quit() { mIsRunning = false; }

		void OnSettingsChange(const Data::Scope& previousSettings, const Data::Scope& currentSettings);
		void InitLightingShaders() const;

	private:
		static void LoadAllAssets();
		void LoadFonts();

		SceneLoader mSceneLoader{};

		bool mIsRunning = true;
		bool mShouldLoadInFonts = true;

		// Keep one instance alive at all times, this means we don't have to load it from file everytime we switch scenes.
		std::unique_ptr<Data::SavedData> mSandboxData{};
		std::unique_ptr<Data::SavedData> mMainMenuData{};
	};
}
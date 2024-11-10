#pragma once

namespace Framework
{
	class Scene;
	class Sprite;

	class SceneLoader
	{
	public:
		SceneLoader();
		~SceneLoader();

		void Init();

		void RequestLoading(std::unique_ptr<Scene> toScene);
		inline bool HasARequestBeenMade() const { return mRequestedScene != nullptr; }

		void StartLoading();

		void ContinueLoading();
		inline bool ShouldContinueLoading() const { return mLoadingProgress != 100 && mActiveScene != nullptr; }

		void DisplayLoadingScreen();

		std::optional<Scene*> GetActiveScene();

	private:
		static constexpr float sLoadingScreenFPS = 60.0f;

		std::unique_ptr<Scene> mActiveScene{};
		std::unique_ptr<Scene> mRequestedScene{};

		std::shared_ptr<Sprite> mLoadingIcon{};

		uchar mLoadingProgress{};
	};
}
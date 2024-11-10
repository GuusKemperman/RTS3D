#include "precomp.h"
#include "SceneLoader.h"

#include <chrono>

#include "TimeManager.h"
#include "AssetManager.h"
#include "Sprite.h"
#include "Scene.h"

Framework::SceneLoader::SceneLoader() = default;
Framework::SceneLoader::~SceneLoader() = default;

void Framework::SceneLoader::Init()
{
	mLoadingIcon = AssetManager::Inst().GetAsset<Sprite>("data/sprites/loadingicon.txt");
}

void Framework::SceneLoader::RequestLoading(std::unique_ptr<Scene> toScene)
{
	mRequestedScene = std::move(toScene);
}

void Framework::SceneLoader::StartLoading()
{
	if (!HasARequestBeenMade())
	{
		return;
	}

	if (mActiveScene != nullptr)
	{
		mActiveScene->Unload();
	}

	mActiveScene = std::move(mRequestedScene);
	mRequestedScene.reset();
	mLoadingProgress = 0;
}

void Framework::SceneLoader::ContinueLoading()
{
	if (!ShouldContinueLoading())
	{
		return;
	}

	constexpr float maxTotalTime = 1.0f / sLoadingScreenFPS;

	std::chrono::high_resolution_clock::time_point t1 = std::chrono::high_resolution_clock::now();
	std::chrono::high_resolution_clock::time_point t2{};

	float timePassed{};

	do
	{
		mLoadingProgress = mActiveScene->Deserialize(mLoadingProgress);

		t2 = std::chrono::high_resolution_clock::now();
		timePassed = (std::chrono::duration_cast<std::chrono::duration<float>>(t2 - t1)).count();
	} while (timePassed < maxTotalTime
		&& mLoadingProgress < 100);
}

void Framework::SceneLoader::DisplayLoadingScreen()
{
	const glm::vec2 windowSize = { sScreenWidth, sScreenHeight };
	ImGui::SetNextWindowSize(Math::ToIMGui(windowSize));
	ImGui::SetNextWindowPos({ 0, 0 });
	
	ImGuiWindowFlags windowFlags{};

	windowFlags |= ImGuiWindowFlags_NoMove;
	windowFlags |= ImGuiWindowFlags_NoResize;
	windowFlags |= ImGuiWindowFlags_NoCollapse;
	windowFlags |= ImGuiWindowFlags_NoTitleBar;
	ImGui::PushStyleColor(ImGuiCol_WindowBg, { 0.0f, 0.0f, 0.0f, 1.0f });

	if (ImGui::Begin("Loading window", nullptr, windowFlags))
	{
		const float timePassed = TimeManager::GetTotalTimePassed();
		const uint numOfFrames = mLoadingIcon->GetNumOfFrames();
		const uint frame = static_cast<uint>(fmodf(timePassed, static_cast<float>(numOfFrames) / static_cast<float>(sLoadingScreenFPS)) * sLoadingScreenFPS);
		mLoadingIcon->SetFrame(frame);

		const glm::vec2 iconSize = mLoadingIcon->GetFrameSizePixels();
		ImGui::SetCursorPos( {(windowSize.x - iconSize.x) * 0.5f, windowSize.y * .2f });
		mLoadingIcon->DrawImGui();

		const ImVec2 loadingBarSize = { windowSize.x * .6f, windowSize.y * .1f };

		ImGui::SetCursorPosX((windowSize.x - loadingBarSize.x) * .5f);
		ImGui::ProgressBar(static_cast<float>(mLoadingProgress) / 100.0f, loadingBarSize);
	}
	ImGui::End();

	ImGui::PopStyleColor();
}

std::optional<Framework::Scene*> Framework::SceneLoader::GetActiveScene()
{
	if (mActiveScene == nullptr)
	{
		return {};
	}

	if (mLoadingProgress < 100)
	{
		return {};
	}
	else
	{
		return mActiveScene.get();
	}
}
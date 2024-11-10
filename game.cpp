#include "precomp.h"
#include "game.h"

#include <filesystem>

#include "TimeManager.h"
#include "InputManager.h"
#include "AssetManager.h"
#include "Scene.h"
#include "MainMenu.h"
#include "SavedData.h"
#include "EntityManager.h"
#include "Settings.h"

// For building the framework's factories
#include "Entity.h"
#include "Chunk.h"
#include "Agent.h"

// For your factories
#include "Tree.h"
#include "Army.h"
#include "Player.h"
#include "Opponent.h"
#include "Unit.h"
#include "Projectile.h"
#include "Turret.h"
#include "Explosion.h"

// All the assets that you want to load in
#include "ImGuiFontWrapper.h"
#include "MyShader.h"
#include "AnimatedMesh.h"
#include "Mesh.h"
#include "Material.h"
#include "ProceduralUnitFactory.h"
#include "Sprite.h"
#include "Texture.h"

// Use this for everything that needs to happen before all the newFrame functions.
Framework::Game::Game()
{
	LoadFonts();
}

Framework::Game::~Game() = default;

// -----------------------------------------------------------
// Called just before Tick, but just after all the new frame functions have been called.
// -----------------------------------------------------------
void Framework::Game::Init()
{
	Settings::Inst().mOnSettingsChanged.bind(this, &Game::OnSettingsChange);

	// Just to keep everything always loaded into memory.
	mSandboxData = std::make_unique<Data::SavedData>("sandbox.txt");
	mMainMenuData = std::make_unique<Data::SavedData>("mainmenu.txt");

	// Framework factories
	EntityManager::BuildFactory<Entity::Factory>();
	EntityManager::BuildFactory<Agent::Factory>();

	// Insert your own factories here
	EntityManager::BuildFactory<RTS::Tree::Factory>();
	EntityManager::BuildFactory<RTS::Army::Factory>();
	EntityManager::BuildFactory<RTS::Player::Factory>();
	EntityManager::BuildFactory<RTS::Opponent::Factory>();
	EntityManager::BuildFactory<RTS::Unit::Factory>();
	EntityManager::BuildFactory<RTS::Projectile::Factory>();
	EntityManager::BuildFactory<RTS::Turret::Factory>();
	EntityManager::BuildFactory<RTS::Explosion::Factory>();

	mSceneLoader.Init();
	mSceneLoader.RequestLoading(std::make_unique<RTS::MainMenu>(*this));

	std::filesystem::create_directory(sDataRoot + "saves");
	std::filesystem::create_directory(sDataRoot + "levels");

	LoadAllAssets();
	InitLightingShaders();
}

void Framework::Game::EarlyTick()
{
	if (mShouldLoadInFonts)
	{
		LoadFonts();
	}
}

// -----------------------------------------------------------
// Main application tick function - Executed once per frame
// -----------------------------------------------------------
bool Framework::Game::Tick( float deltaTime)
{
	Framework::TimeManager::UpdateDeltaTime(deltaTime);

	if (mSceneLoader.HasARequestBeenMade())
	{
		mSceneLoader.StartLoading();
	}

	if (mSceneLoader.ShouldContinueLoading())
	{
		mSceneLoader.ContinueLoading();
		mSceneLoader.DisplayLoadingScreen();
	}
	else
	{
		std::optional<Scene*> activeScene = mSceneLoader.GetActiveScene();

		if (activeScene.has_value())
		{
			activeScene.value()->Tick();
			activeScene.value()->Draw();
		}
	}
 
	return mIsRunning;
}

void Framework::Game::Shutdown()
{
	Framework::AssetManager::Inst().Clear();

	Settings::Inst().mOnSettingsChanged.unbind(this, &Game::OnSettingsChange);
}

void Framework::Game::RequestLoadTo(std::unique_ptr<Framework::Scene> scene)
{
	mSceneLoader.RequestLoading(std::move(scene));
}

void Framework::Game::OnSettingsChange(const Data::Scope& previousSettings, const Data::Scope& currentSettings)
{
	mShouldLoadInFonts = previousSettings.GetVariable("fontQuality") != currentSettings.GetVariable("fontQuality");

	if (previousSettings.GetVariable("specularIntensity") != currentSettings.GetVariable("specularIntensity"))
	{
		InitLightingShaders();
	}
}

void Framework::Game::InitLightingShaders() const
{
	AssetManager& am = AssetManager::Inst();
	std::array<std::shared_ptr<MyShader>, 3> shadersWithLighting{};
	shadersWithLighting[0] = am.GetAsset<MyShader>("shaders/terrain.vert,shaders/terrain.frag");
	shadersWithLighting[1] = am.GetAsset<MyShader>("shaders/animated.vert,shaders/standard.frag");
	shadersWithLighting[2] = am.GetAsset<MyShader>("shaders/standard.vert,shaders/standard.frag");

	constexpr glm::vec3 sLightColor = { 0.945f, 0.855f, .643f };
	constexpr glm::vec3 sAmbientColor = sLightColor * 0.5f;
	const glm::vec3 sLightDir = glm::normalize(glm::vec3{ -0.5773502691896258f, 0.5773502691896258f, -0.5773502691896258f }); // Needs to be unit vector
	float specularStrength;
	Settings::Inst().GetSettings().GetVariable("specularIntensity") >> specularStrength;

	// Since we dont change the light color or direction, let's just set these at the start
	for (std::shared_ptr<MyShader>& shader : shadersWithLighting)
	{
		shader->Bind();
		shader->SetFloat3("lightColor", sLightColor);
		shader->SetFloat3("ambientLight", sAmbientColor);
		shader->SetFloat3("lightDirection", sLightDir);
		shader->SetFloat("specularStrength", specularStrength);
		shader->Unbind();
	}
}

// It doesnt need to be here, but it'd be nice if errors with loading would be triggered immediately.
void Framework::Game::LoadAllAssets()
{
	AssetManager& am = AssetManager::Inst();

	am.GetAsset<MyShader>("shaders/debugshader.vert,shaders/debugshader.frag");
	am.GetAsset<MyShader>("shaders/terrain.vert,shaders/terrain.frag");
	am.GetAsset<MyShader>("shaders/animated.vert,shaders/standard.frag");
	am.GetAsset<MyShader>("shaders/standard.vert,shaders/standard.frag");

	am.GetAsset<Material>("materials/terrain.mtl")->LoadWithoutAssimp();

	am.GetAsset<RTS::ProceduralUnitFactory>(sDataRootWithoutAssetRoot + "procedural");

	for (size_t i = 0; i < RTS::Tree::sNumOfTreeModels; i++)
	{
		am.GetAsset<Framework::Mesh>("models/tree" + std::to_string(i) + ".obj");
	}

	am.GetAsset<Framework::AnimatedMesh>("models/explosion.dae");

	am.GetAsset<Framework::Mesh>("models/selectedindicator.obj");
	am.GetAsset<Framework::Mesh>("models/highlightedindicator.obj");
	am.GetAsset<Framework::Mesh>("models/enemyhighlightedindicator.obj");
	am.GetAsset<Framework::Mesh>("models/projectile.obj");

	am.GetAsset<Sprite>("data/sprites/endscreen.txt");

	// Not necesarry, but we're assuming now that everything has been loaded in.
	// If we're trying to load something else in, this would probably mean that we 
	// specified the wrong name/type/arguments and an additional asset will be loaded
	// instead of being shared.
	am.LockFurtherLoading();
}

void Framework::Game::LoadFonts()
{
	ImFontAtlas* fonts = ImGui::GetIO().Fonts;
	fonts->Clear();


	AssetManager& am = AssetManager::Inst();
	am.GetAsset<Framework::ImGuiFontWrapper>("fonts/courbd.ttf,40.0f")->SendToImguiFontAtlas();
	am.GetAsset<Framework::ImGuiFontWrapper>("fonts/ALGER.TTF,200.0f")->SendToImguiFontAtlas();
	
	fonts->Build();

	{
		// Continously try to make lower quality version's until we have one that our gpu supports, because we cannot be bothered by doing the math and this is only done once: the first time this is played on a new device.
		unsigned char* pixels{};
		int width, height;
		fonts->GetTexDataAsAlpha8(&pixels, &width, &height);
		int maxSize = Texture::GetMaxTextureSizeForDevice();
		if (width > maxSize 
			|| height > maxSize)
		{
			float maxQuality;
			Framework::Data::Scope settingsScope = Framework::Settings::Inst().GetSettings();
			Framework::Data::Variable& fontQualitySetting = settingsScope.GetVariable("maxFontQuality");
			fontQualitySetting >> maxQuality;
			maxQuality -= 0.1f;
			fontQualitySetting << maxQuality;
			Framework::Settings::Inst().SetSettings(settingsScope);
			LOGMESSAGE("Imgui fontatlas size: " << width << "x" << height << " was too big, attempting to make a smaller version.");
			LoadFonts();
		}
	}

	ImGui_ImplOpenGL3_DestroyDeviceObjects();

	mShouldLoadInFonts = false;
}
#pragma once
#include <atomic>

namespace Framework
{
	namespace Data
	{
		class SavedData;
		class Scope;
	}

	class Game;
	class EntityManager;
	class Camera;
	class Terrain;
	class Physics;

	class Scene
	{
	public:
		// Provide levelFile and levelName if you want the scene to be loading in from file on load.
		Scene(Game& game, const std::string& levelFile = "", const std::string& levelName = "");
		virtual ~Scene();

		Scene(const Scene&) = delete;
		Scene& operator=(const Scene&) = delete;

		virtual uchar Deserialize(const uchar progress);

		virtual void Tick();
		
		// Not const, since we're drawing imgui and seeing if the player interacted with the ui at the same time.
		virtual void Draw();
		virtual void Unload();
		virtual void DrawImGui() {};

		void Serialize(const std::string& saveName) const;

		Game& mGame;

		std::unique_ptr<Physics> mPhysics{};
		std::unique_ptr<EntityManager> mEntityManager{};
		std::unique_ptr<Camera> mCamera{};
		std::unique_ptr<Terrain> mTerrain{};

	protected:
		std::unique_ptr<Framework::Data::SavedData> mSceneData{};

		virtual void Serialize(Data::Scope& parentScope) const;
	};
}
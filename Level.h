#pragma once
#include "Scene.h"

namespace Framework
{
	class Sprite;

	namespace Data
	{
		class SavedData;
		class Scope;
	}
}

namespace RTS
{
	class Forest;
	class Army;

	class Level :
		public Framework::Scene
	{
	public:
		Level(Framework::Game& game, const std::string& levelFile, const std::string& levelName);
		~Level();

		void Tick() override;

		uchar Deserialize(const uchar progress) override;
		void DrawImGui() override;

		void Unload() override;

	protected:
		const std::optional<const Framework::Data::Scope*>& GetScopeUsedForLevelGeneration() const { return mLevelGeneration; }

		Army* mPlayerArmy{};
		Army* mOpponentArmy{};

	private:
		std::string GenerateSaveName() const;

		void TogglePause();

		bool mIsPaused{};

		std::optional<const Framework::Data::Scope*> mLevelGeneration{};

		// Only needed for spawning the trees.
		std::unique_ptr<Forest> mForest{};

		std::string mWhatToNameTheSave{};

		enum class VictoryState { none = -1, opponentWon, playerWon };
		VictoryState mVictoryState = VictoryState::none;
		std::shared_ptr<Framework::Sprite> mEndscreen{};
	};
}
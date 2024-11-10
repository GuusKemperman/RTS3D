#pragma once
#include "Entity.h"
#include "BoundingBox2D.h"
#include "CameraControllers.h"

namespace RTS
{
	class Army;
	class Unit;

	class Player :
		public Framework::Entity
	{
		ENTITYMAKER(Player);
	public:
		Player(Framework::Scene& scene, Framework::EntityId armyEntityId = 0);

		void Tick() override;

		bool Serialize(Framework::Data::Scope& parentScope) const;
		void Deserialize(const Framework::Data::Scope& parentScope);

		inline void SetArmy(Army* army) { mArmy = army; }

	private:
		void Draw() const;

		void UpdateWhichUnitsAreSelected();

		void UpdateSelectingArea();
		std::vector<Unit*> CheckForUnits(const glm::vec2 atScreenPosition) const;
		std::vector<Unit*> CheckForUnits(const Framework::BoundingBox2D& inBox) const;

		// Does not preserve order of vector!
		void RemoveUnselectableUnits(std::vector<Unit*>& fromVector) const;

		Army* mArmy{};

		// In screenspace
		std::optional<glm::vec2> mSelectingAreaStart{};
		// In screenspace
		std::optional<Framework::BoundingBox2D> mSelectingArea{};

		using Selection = std::vector<Framework::EntityId>;
		std::array<Selection, 10> mStoredSelections{};
		Selection mSelection{};

		// Can be stored using a raw pointer, since it gets recalculated each frame.
		std::vector<Unit*> mHighlightedUnits{};

		ushort mSelectedIndicatorMeshId{};
		ushort mHighlightedIndicatorMeshId{};
		ushort mEnemyHighlightedIndicatorMeshId{};

		TopDownCameraController mCameraController;
	};
}
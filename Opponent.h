#pragma once
#include "Entity.h"

namespace RTS
{
	class Army;
	class Unit;

	class Opponent :
		public Framework::Entity
	{
		ENTITYMAKER(Opponent);
	public:
		Opponent(Framework::Scene& scene, Framework::EntityId armyEntityId = 0);

		void FixedTick() override;

		bool Serialize(Framework::Data::Scope& parentScope) const;
		void Deserialize(const Framework::Data::Scope& parentScope);

		inline void SetArmy(Army* army) { mArmy = army; }

	private:
		void OrderToRandomPosition(std::vector<Unit*>&& group) const;
		uint RandomGroupSize() const;

		Army* mArmy{};

		static constexpr uint sMaxGroupSize = 50u;
		static constexpr uint sMinGroupSize = 10u;

		static constexpr float sMinDistFromEdge = 10.0f;
		static constexpr float sGiveCommandChance = .05f;
	};
}
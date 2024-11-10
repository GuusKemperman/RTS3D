#pragma once
#include "Agent.h"
#include "Commands.h"

namespace RTS
{
	class Army;
	struct UnitBodyData;

	class Unit :
		public Framework::Agent
	{
		ENTITYMAKER(Unit);
	public:
		Unit(Framework::Scene& scene, Army* army = nullptr);
		virtual ~Unit();

		// Units will not be part of an army immediately after deserialization, the useArmyId is needed to select the right mesh.
		void SetUnitBodyData(const UnitBodyData* data, const std::optional<ArmyId> useArmyId = {});

		void Tick() override;
		AgentInput CalculateDesiredVelocity() override;

		template<typename CommandType, typename ...Args>
		void GiveCommand(Args&& ...args)
		{
			mCommand = std::make_unique<CommandType>(std::forward<Args>(args)...);
		}
		inline AggroLevel GetAggroLevel() const { return mAggroLevel; }
		inline void SetAggroLevel(AggroLevel level) { mAggroLevel = level; }

		ArmyId GetArmyId() const;
		virtual void SetArmy(Army* army);

		void ReceiveDamage(float byAmount);

		bool Serialize(Framework::Data::Scope& parentScope) const override;
		void Deserialize(const Framework::Data::Scope& parentScope) override;

		std::optional<Unit*> CheckForUnitToAttack();
		std::vector<Unit*> GetUnitsInSight();

		template<typename CommandType, typename ...Args>
		bool AttemptTransition(bool condition, Args && ...args)
		{
			if (condition)
			{
				GiveCommand<CommandType>(std::forward<Args>(args)...);
				mSwitchedState = true;
			}
			return condition;
		}

		static constexpr float sSightRange = 50.0f;

	private:
		std::unique_ptr<Command> mCommand{};
		//union { Command mCommandBase; CommandIdle mIdleCommand{}; CommandMoveTo mMoveToCommand; CommandAttack mAttackCommand; CommandInvestigate mInvestigateCommand; };
		AggroLevel mAggroLevel = AggroLevel::pursuit;

		const Army* mArmy{};
		const UnitBodyData* mUnitBodyData{};

		Framework::Inquirer mSightInquirer{};

		float mHealth = 1.0f;
		bool mSwitchedState{};
	};
}
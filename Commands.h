#pragma once
#include "Agent.h"

namespace RTS
{
	class Unit;

	struct Command
	{
		Command() = default;
		virtual ~Command() = default;
		virtual Framework::Agent::AgentInput CalculateAgentInput(Unit* unit) const = 0;
		virtual CommandType GetType() const = 0;
	};

	struct CommandIdle :
		public Command
	{
		Framework::Agent::AgentInput CalculateAgentInput(Unit* unit) const override;
		inline CommandType GetType() const override { return CommandType::idle; }
	};

	struct CommandMoveTo :
		public Command
	{
		CommandMoveTo() = default;
		CommandMoveTo(const glm::vec2& position, std::optional<glm::vec2> desiredForward = {}) : mToPosition(position), mDesiredForward(desiredForward) {};
		Framework::Agent::AgentInput CalculateAgentInput(Unit* unit) const override;
		inline CommandType GetType() const override { return CommandType::moveTo; }

		glm::vec2 mToPosition{};
		std::optional<glm::vec2> mDesiredForward{};
	};

	struct CommandAttack :
		public Command
	{
		CommandAttack() = default;
		CommandAttack(Framework::EntityId target) : mTarget(target) {};
		Framework::Agent::AgentInput CalculateAgentInput(Unit* unit) const override;
		inline CommandType GetType() const override { return CommandType::attack; }

		Framework::EntityId mTarget{};
	};

	std::vector<glm::vec2> GenerateFormation(const std::vector<Unit*>& units, const glm::vec2 position);

	void FormFormation(std::vector<Unit*> units, const glm::vec2 position, const std::optional<float> rotation = {});
}
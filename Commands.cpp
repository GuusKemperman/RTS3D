#include "precomp.h"
#include "Commands.h"

#include "Unit.h"
#include "Scene.h"
#include "EntityManager.h"

glm::vec2 GetCentre(const std::vector<RTS::Unit*>& units)
{
	glm::vec2 groupCentre{};

	for (RTS::Unit* unit : units)
	{
		groupCentre += unit->GetTransform().GetLocalPosition2D();
	}

	return groupCentre / static_cast<float>(units.size());
}

namespace RTS
{
	void FormUniformFormation(std::vector<Unit*> units, const glm::vec2 position, const std::optional<float> rotation);
}

std::vector<glm::vec2> RTS::GenerateFormation(const std::vector<Unit*>& units, const glm::vec2 position)
{
	if (units.empty())
	{
		return {};
	}

	// Generate points
	//Unit* biggestUnit = *std::max_element(units.begin(), units.end(),
	//	[](const Unit* a, const Unit* b)
	//	{
	//		return a->GetRadius() < b->GetRadius();
	//	});

	const float spacing = /*biggestUnit->GetRadius() **/ 10.0f;

	// Generate points
	std::vector<glm::vec2> points(units.size());
	uint totalNumberOfPoints{};

	float distFromCentre = 0.0f;
	do
	{
		const uint numberOfPointsInLayer = static_cast<uint>(PI / asin(spacing / distFromCentre)) + 1;

		const float angleStepSize = 360.0f / static_cast<float>(numberOfPointsInLayer);

		for (uint pointNum = 0; pointNum < numberOfPointsInLayer && totalNumberOfPoints < units.size(); pointNum++, totalNumberOfPoints++)
		{
			const float angle = angleStepSize * static_cast<float>(pointNum);
			points[totalNumberOfPoints] = { position + Framework::Math::AngleToVec2(angle) * distFromCentre };
		}

		distFromCentre += spacing;
	} while (totalNumberOfPoints < units.size());

	return points;
}

void RTS::FormFormation(std::vector<Unit*> units, const glm::vec2 position, const std::optional<float> rotation)
{
	FormUniformFormation(std::move(units), position, rotation);
}

// the vector of units passed in consists of either air or ground units, not a mix of both.
void RTS::FormUniformFormation(std::vector<Unit*> units, const glm::vec2 position, const std::optional<float> rotation)
{
	const std::vector<glm::vec2> points = GenerateFormation(units, position);
	const glm::vec2 groupCentre = GetCentre(units);

	const glm::vec2 deltaPosGroup = position - groupCentre;
	const std::optional<glm::vec2> unitsRotation = Framework::Math::AngleToVec2(rotation.value_or(Framework::Math::Vec2ToAngle(position - groupCentre)));

	CommandMoveTo moveToCommand{};

	for (glm::vec2 point : points)
	{
		const glm::vec2 pickClosestToPoint = point - deltaPosGroup;

		auto closestIt = std::min_element(units.begin(), units.end(),
			[pickClosestToPoint](const Unit* a, const Unit* b)
			{
				return glm::distance2(a->GetTransform().GetLocalPosition2D(), pickClosestToPoint) < glm::distance2(b->GetTransform().GetLocalPosition2D(), pickClosestToPoint);
			});

		moveToCommand.mToPosition = point;

		(*closestIt)->GiveCommand<CommandMoveTo>(point, unitsRotation);
			
		*closestIt = units.back();
		units.pop_back();
	}
}

Framework::Agent::AgentInput RTS::CommandIdle::CalculateAgentInput(Unit* unit) const
{
	if (unit->GetAggroLevel() != AggroLevel::pursuit)
	{
		return Framework::Agent::AgentInput{};
	}

	std::optional<Unit*> canAttackUnit = unit->CheckForUnitToAttack();

	if (canAttackUnit.has_value()
		&& unit->AttemptTransition<CommandAttack>(canAttackUnit.has_value(), canAttackUnit.value()->GetId()))
	{
		return unit->CalculateDesiredVelocity();
	}

	return Framework::Agent::AgentInput{};
}

Framework::Agent::AgentInput RTS::CommandMoveTo::CalculateAgentInput(Unit* unit) const
{
	if (unit->GetAggroLevel() == AggroLevel::pursuit)
	{
		std::optional<Unit*> canAttackUnit = unit->CheckForUnitToAttack();

		if (canAttackUnit.has_value()
			&& unit->AttemptTransition<CommandAttack>(canAttackUnit.has_value(), canAttackUnit.value()->GetId()))
		{
			return Framework::Agent::AgentInput{};
		}
	}

	const glm::vec2 arrivalVel = unit->CalculateArrival(mToPosition);

	if (arrivalVel == glm::vec2{ 0.0f })
	{
		if (unit->AttemptTransition<CommandIdle>(!mDesiredForward.has_value()))
		{
			return Framework::Agent::AgentInput{};
		}

		const glm::vec3 currentForward = unit->GetTransform().GetLocalForward();
		const glm::vec2 desiredForward2D = mDesiredForward.value();

		const float turningLeft = glm::dot(glm::vec3{ desiredForward2D.x, currentForward.y, desiredForward2D.y }, currentForward);

		if (unit->AttemptTransition<CommandIdle>(turningLeft >= 0.90f))
		{
			return Framework::Agent::AgentInput{};
		}

		// Make sure that the unit turns in the right direction
		return Framework::Agent::AgentInput{ {}, mDesiredForward };
	}

	const glm::vec2 avoidanceVel = unit->CalculateAvoidance();

	// Prioritise avoiding others over reaching your destination
	const glm::vec2 combinedVel = unit->CombineVelocities(avoidanceVel, arrivalVel);

	return Framework::Agent::AgentInput{ combinedVel };
}

Framework::Agent::AgentInput RTS::CommandAttack::CalculateAgentInput(Unit* unit) const
{
	// But use a static cast for speed reasons
	Unit* target = dynamic_cast<Unit*>(unit->GetScene().mEntityManager->TryGetEntity(mTarget).value_or(nullptr));

	// Pick new target
	if (target == nullptr)
	{
		// Only pick new target if investigating or aggresive
		if (unit->AttemptTransition<CommandIdle>(unit->GetAggroLevel() == AggroLevel::holdFire))
		{
			return Framework::Agent::AgentInput{};
		}

		std::optional<Unit*> canAttackUnit = unit->CheckForUnitToAttack();

		if (unit->AttemptTransition<CommandIdle>(!canAttackUnit.has_value()))
		{
			return Framework::Agent::AgentInput{};
		}

		target = canAttackUnit.value();
	}

	// Move towards target if too far away
	const glm::vec2 myPosition = unit->GetTransform().GetLocalPosition2D();
	const glm::vec2 targetPosition = target->GetTransform().GetLocalPosition2D();

	glm::vec2 deltaPos = targetPosition - myPosition;
	float sqrdDistanceToTarget = glm::length2(deltaPos);

	if (sqrdDistanceToTarget == 0.0f)
	{
		deltaPos = { 0.01f, 0.01f };
		sqrdDistanceToTarget = glm::length2(deltaPos);
	}

	constexpr float sqrdMaxDesiredDist = Framework::Math::sqr(Unit::sSightRange);
	constexpr float sqrdMinDesiredDist = Framework::Math::sqr(Framework::Agent::sAvoidanceRange * 1.5f);

	static_assert(sqrdMaxDesiredDist > sqrdMinDesiredDist);

	const float distAsPercentage = Framework::Math::lerpInv(sqrdMinDesiredDist, sqrdMaxDesiredDist, sqrdDistanceToTarget);
	const float seekScalar = glm::clamp(distAsPercentage, 0.0f, 1.0f);
	const glm::vec2 normalizedDeltaPos = deltaPos / sqrtf(sqrdDistanceToTarget);

	if (seekScalar <= 0.25f)
	{
		return Framework::Agent::AgentInput{ {}, normalizedDeltaPos };
	}

	const glm::vec2 avoidance = unit->CalculateAvoidance();
	const glm::vec2 seek = normalizedDeltaPos * seekScalar;

	const glm::vec2 combinedVel = Framework::Agent::CombineVelocities(avoidance, seek);
	return Framework::Agent::AgentInput{ combinedVel };
}
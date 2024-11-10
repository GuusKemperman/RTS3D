#pragma once
#include "Entity.h"
#include "Inquirer.h"

namespace Framework
{
    class Agent :
        public Entity
    {
        ENTITYMAKER(Agent);
    public:
        Agent(Scene& scene);
        virtual ~Agent();

        void Tick() override;
        void FixedTick() override;

        // Needed for updating the rigid body position as well, and sets the agent to the correct height as well.
        void ForceSetPosition(const glm::vec2 position);

        bool Serialize(Framework::Data::Scope& parentScope) const;
        void Deserialize(const Framework::Data::Scope& parentScope);

        bool IsInRagdollState() const;

        struct AgentInput
        {
            glm::vec2 mDesiredVelocity{};

            // Only gets evaluated if velocity is zero.
            std::optional<glm::vec2> mDesiredForward{};
        };

        // Returns vector with max length of 1.0f.
        glm::vec2 CalculateAvoidance();
        glm::vec2 CalculateSeek(const glm::vec2 towardPosition) const;
        glm::vec2 CalculateArrival(const glm::vec2 arriveAt) const;
        glm::vec2 CalculateWander() const;

        // The recessive velocity will only be expressed when the dominant velocity has a length smaller than 1.0f.
        // The returned velocity is guarenteed to be no longer than 1.0f;
        static glm::vec2 CombineVelocities(const glm::vec2& dominantVelocity, const glm::vec2& recessiveVelocity);

        static constexpr float sAvoidanceRange = 10.0f;

    protected:
        float CalculateDesiredHeight(const glm::vec2& atPosition) const;

        float mHoverAtHeight{};
        float mTurnSpeed{};
        float mMovementSpeed{};
        
    private:
        virtual AgentInput CalculateDesiredVelocity() { return { {0.0f, 0.0f}, {} }; }

        float CalculateAmountOfTraction() const;
        glm::quat CalculateDesideredOrientation() const;

        Inquirer mAvoidanceInquirer{};

        static constexpr float sWanderChangeSensitivity = 2.0f;

        AgentInput mLastInput{};
        glm::vec3 mVelocity{};
    };
}
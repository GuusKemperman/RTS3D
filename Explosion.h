#pragma once
#include "Entity.h"

namespace Framework
{
    class AnimatedMesh;
    class Animator;
}

namespace RTS
{
    class Explosion :
        public Framework::Entity
    {
        ENTITYMAKER(Explosion);
    public:
        Explosion(Framework::Scene& scene, const Framework::Transform& transform = {}, const float explosionForce = 0.0f);
        ~Explosion();

        void Tick() override;
        void Draw() const override;

        bool Serialize(Framework::Data::Scope& parentScope) const override;
        void Deserialize(const Framework::Data::Scope& parentScope) override;

    private:
        void ApplyForcesAndDamage(const float explosionRadius, const float explosionForce);

        std::shared_ptr<Framework::AnimatedMesh> mMesh{};
        std::unique_ptr<Framework::Animator> mAnimator{};
        btSphereShape mCollisionShape;

        float mGrowSpeed = 4.0f;
    };
}
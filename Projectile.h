#pragma once
#include "Entity.h"

namespace RTS
{
	class Projectile :
		public Framework::Entity
	{
		ENTITYMAKER(Projectile);
	public:
		Projectile(Framework::Scene& scene, const glm::vec3& position = {0.0f, 0.0f, 0.0f}, const glm::vec3& velocity = {0.0f, 0.0f, 0.0f}, const float explosionForce = 0.0f);
		~Projectile();

		void FixedTick() override;

		bool Serialize(Framework::Data::Scope& parentScope) const override;
		void Deserialize(const Framework::Data::Scope& parentScope) override;

		void OnCollision(const btCollisionObject* object) override;

	private:
		void Hit();

		static constexpr float sDirectHitDamage = 0.1f;
		float mExplosionForce{};
		bool mHasBeenDestroyed{};
	};
}
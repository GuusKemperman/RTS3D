#include "precomp.h"
#include "Projectile.h"

#include "AssetManager.h"
#include "Mesh.h"
#include "Scene.h"
#include "Terrain.h"
#include "Unit.h"
#include "Physics.h"
#include "Explosion.h"
#include "EntityManager.h"
#include "Inquirer.h"
#include "Scope.h"

RTS::Projectile::Projectile(Framework::Scene& scene, const glm::vec3& position, const glm::vec3& velocity, const float explosionForce) :
    Framework::Entity(scene),
    mExplosionForce(explosionForce)
{
    mMeshId = Framework::AssetManager::Inst().GetAsset<Framework::Mesh>("models/projectile.obj")->GetMeshId();

    Framework::Transform& myTransform = GetTransform();
    myTransform.SetLocalPosition(position);
    myTransform.SetLocalForward(glm::normalize(velocity));

    static btBoxShape* shape{};
    static btVector3 inertia{};

    constexpr float mass = .2f;

    if (shape == nullptr)
    {
        shape = new btBoxShape({ 0.1f, 0.1f, 0.5f });
        shape->calculateLocalInertia(mass, inertia);
    }

    std::unique_ptr<btRigidBody> rigidBody = std::make_unique<btRigidBody>(mass, &myTransform, shape, inertia);

    rigidBody->setFriction(100000.0f);
    rigidBody->setLinearVelocity(Framework::Math::ToBullet(velocity));

    mCollisionObject = std::move(rigidBody);
    mCollisionObject->setUserPointer(this);

    mScene.mPhysics->AddCollisionObjectToWorld(mCollisionObject.get(), Framework::Physics::Group::projectileGroup, Framework::Physics::Mask::projectileMask);

    mHasCollisionCallback = true;
    mHasFixedTick = true;
}

RTS::Projectile::~Projectile()
{
    mScene.mPhysics->RemoveCollisionObjectFromWorld(std::move(mCollisionObject));
}

void RTS::Projectile::FixedTick()
{
    const btRigidBody* rigidbody = static_cast<btRigidBody*>(mCollisionObject.get());
    const glm::vec3 currentLinearVelocity = Framework::Math::ToGLM(rigidbody->getLinearVelocity());

    Framework::Transform& myTransform = GetTransform();
    myTransform.SetLocalForward(currentLinearVelocity);

    const glm::vec3 myPosition = myTransform.GetLocalPosition();
    const Framework::TerrainData* terrainData = mScene.mTerrain->GetData();
    if (!terrainData->mWorldBounds.Contains({ myPosition.x, myPosition.z }))
    {
        Hit();
    }
}

bool RTS::Projectile::Serialize(Framework::Data::Scope& parentScope) const
{
    Entity::Serialize(parentScope);

    parentScope.AddChild("Projectile").AddVariable("explosionForce") << mExplosionForce;

    return true;
}

void RTS::Projectile::Deserialize(const Framework::Data::Scope& parentScope)
{
    Entity::Deserialize(parentScope);

    parentScope.GetScope("Projectile").GetVariable("explosionForce") >> mExplosionForce;
}

void RTS::Projectile::OnCollision(const btCollisionObject* object)
{
    Hit();

    Unit* unit = dynamic_cast<Unit*>(static_cast<Entity*>(object->getUserPointer()));

    if (unit != nullptr)
    {
        unit->ReceiveDamage(sDirectHitDamage);
    }
}

void RTS::Projectile::Hit()
{
    // Make sure we don't destroy ourselves multiple times, if we hit multiple objects in one frame.
    if (mHasBeenDestroyed)
    {
        return;
    }

    const Framework::Transform& myTransform = GetTransform();
    mScene.mEntityManager->AddEntity<Explosion>(myTransform, mExplosionForce);

    Destroy();
    mHasBeenDestroyed = true;
}
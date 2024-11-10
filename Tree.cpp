#include "precomp.h"
#include "Tree.h"

#include "Scene.h"
#include "Terrain.h"
#include "Physics.h"
#include "AssetManager.h"
#include "Mesh.h"

RTS::Tree::Tree(Framework::Scene& scene, const glm::vec2 position) :
	Entity(scene)
{
	mMeshId = Framework::AssetManager::Inst().GetAsset<Framework::Mesh>("models/tree" + std::to_string(rand() % sNumOfTreeModels) + ".obj")->GetMeshId();

	const float heightAtPosition = mScene.mTerrain->GetHeightAtPosition(position.x, position.y);
	
	const float scale = Framework::Random::Range(sMinScale, sMaxScale);

	Framework::Transform& myTransform = GetTransform();
	myTransform.SetLocalPosition(position.x, heightAtPosition, position.y);
	myTransform.SetLocalOrientation(Framework::Random::Range(-sMaxRotationXZ, sMaxRotationXZ), Framework::Random::Range(TWOPI), Framework::Random::Range(-sMaxRotationXZ, sMaxRotationXZ));
	myTransform.SetLocalScale(scale, scale, scale);

	static btCylinderShape* shape = new btCylinderShape({ 0.5f, 3.0f, 0.5f});
	
	mCollisionObject = std::make_unique<btCollisionObject>();
	mCollisionObject->setCollisionShape(shape);
	myTransform.TranslateLocalPosition(myTransform.GetLocalUp() * 0.5f * 0.5f);
	mCollisionObject->setWorldTransform(myTransform.ToBullet());
	mCollisionObject->setUserPointer(this);

	mScene.mPhysics->AddCollisionObjectToWorld(mCollisionObject.get(), Framework::Physics::Group::staticObstacleGroup, Framework::Physics::Mask::staticObstacleMask);
}

RTS::Tree::~Tree()
{
	mScene.mPhysics->RemoveCollisionObjectFromWorld(std::move(mCollisionObject));
}

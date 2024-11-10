#include "precomp.h"
#include "Transform.h"

#include "Scope.h"
#include "Entity.h"
#include "Scene.h"
#include "EntityManager.h"

#include "Camera.h"

Framework::Transform::Transform() = default;

Framework::Transform::Transform(Entity* owner) :
	mOwner(owner)
{
	
}

Framework::Transform::~Transform()
{
	SetParent(nullptr);

	// The children deattach themselves from their parent, to prevent modifying the array while iterating over it, make a copy.
	std::vector<Transform*> childrenCopy = mChildren;

	for (Transform* child : childrenCopy)
	{
		child->SetParent(nullptr);
	}
}

Framework::Transform::Transform(const Transform& other)
{
	mLocalOrientation = other.mLocalOrientation;
	mLocalPosition = other.mLocalPosition;
	mLocalScale = other.mLocalScale;
}

void Framework::Transform::operator=(const Transform& other)
{
	mLocalOrientation = other.mLocalOrientation;
	mLocalPosition = other.mLocalPosition;
	mLocalScale = other.mLocalScale;
}

glm::vec2 Framework::Transform::GetLocalForward2D() const
{
	const glm::vec3 forward3D = GetLocalForward();
	return glm::normalize(glm::vec2{ forward3D.x, forward3D.z });
}

btTransform Framework::Transform::ToBullet() const
{
	btTransform t;
	glm::mat4 worldMat = GetWorldMatrix();
	t.setFromOpenGLMatrix(&worldMat[0][0]);
	return t; 
}

void Framework::Transform::getWorldTransform(btTransform& worldTrans) const
{
	glm::mat4 worldMatrix = GetWorldMatrix();
	worldTrans.setFromOpenGLMatrix(&worldMatrix[0][0]);
}

void Framework::Transform::setWorldTransform(const btTransform& worldTrans)
{
	SetLocalOrientation(Math::ToGLM(worldTrans.getRotation()));
	SetLocalPosition(Math::ToGLM(worldTrans.getOrigin()));
}

glm::mat4 Framework::Transform::GetLocalMatrix() const
{
    // Scales first, rotates second, translates last.
    glm::mat4 translationMatrix = glm::translate(glm::mat4{ 1.0f }, mLocalPosition);
    glm::mat4 scaleMatrix = glm::scale(glm::mat4{ 1.0f }, mLocalScale);
    glm::mat4 rotationMatrix = glm::toMat4(mLocalOrientation);
    
    return translationMatrix * rotationMatrix * scaleMatrix;
}

glm::mat4 Framework::Transform::GetWorldMatrix() const
{
	if (mParent == nullptr)
	{
		return GetLocalMatrix();
	}

	return mParent->GetWorldMatrix() * GetLocalMatrix();
}

void Framework::Transform::SetLocalForward(const glm::vec3 forward)
{
	SetLocalOrientation(CalculateRotationBetweenOrientations(glm::vec3{ 0.0f, 0.0f, 1.0f }, forward));
}

void Framework::Transform::SetLocalUp(const glm::vec3 up)
{
	SetLocalOrientation(CalculateRotationBetweenOrientations(glm::vec3{ 0.0f, 1.0f, 0.0f }, up));
}

void Framework::Transform::SetLocalRight(const glm::vec3 right)
{
	SetLocalOrientation(CalculateRotationBetweenOrientations(glm::vec3{ 1.0f, 0.0f, 0.0f }, right));
}

void Framework::Transform::SetParent(Transform* parent)
{
	if (mParent == parent)
	{
		return;
	}

	if (mParent != nullptr)
	{
		mParent->DetachChild(this);
	}

	mParent = parent;

	if (mParent != nullptr)
	{
		mParent->AttachChild(this);
	}
}

glm::quat Framework::Transform::GetWorldOrientation() const
{
	if (mParent == nullptr)
	{
		return GetLocalOrientation();
	}

	return mParent->GetWorldOrientation() * GetLocalOrientation();
}

void Framework::Transform::AttachChild(Transform* child)
{
	assert(std::find(mChildren.begin(), mChildren.end(), child) == mChildren.end());
	mChildren.push_back(child);
}

void Framework::Transform::DetachChild(Transform* child)
{
	auto it = std::find(mChildren.begin(), mChildren.end(), child);
	assert(it != mChildren.end());

	mChildren.erase(it);
}

void Framework::Transform::Draw(Camera& camera) const
{
    glm::vec3 lineStart = GetWorldPosition();
	glm::vec3 forward = GetWorldForward();
	glm::vec3 right = GetWorldRight();
	glm::vec3 up = GetWorldUp();

    camera.RequestDebugLineDraw(lineStart, lineStart + forward, glm::vec3{ 0.0f, 0.0f, 1.0f });
    camera.RequestDebugLineDraw(lineStart, lineStart + right, glm::vec3{ 1.0f, 0.0f, 0.0f });
    camera.RequestDebugLineDraw(lineStart, lineStart + up, glm::vec3{ 0.0f, 1.0f, 0.0f });
}

void Framework::Transform::SetWorldOrientation(const glm::quat orientation)
{
	if (mParent == nullptr)
	{
		SetLocalOrientation(orientation);
	}
	else
	{
		const glm::quat parentWorldOrienation = mParent->GetWorldOrientation();
		SetLocalOrientation(glm::inverse(parentWorldOrienation) * orientation);
	}
}

// Stolen from http://www.opengl-tutorial.org/intermediate-tutorials/tutorial-17-quaternions/
glm::quat Framework::Transform::CalculateOrientationTowards(glm::quat current, glm::quat target, const float maxAngle)
{
	if (maxAngle == 0.0f)
	{
		return current;
	}

	current = glm::normalize(current);
	target = glm::normalize(target);

	// Weirdly returns values below -1.0 sometimes, dont judge me for this dumb but easy fix.
	float cosTheta = glm::max(glm::dot(current, target), -1.0f);

	// q1 and q2 are already equal.
	// Force q2 just to be sure
	if (cosTheta > 0.9999f) 
	{
		return target;
	}

	// Avoid taking the long path around the sphere
	if (cosTheta < 0) 
	{
		current = current * -1.0f;
		cosTheta *= -1.0f;
	}

	float angle = acos(cosTheta);

	// If there is only a 2&deg; difference, and we are allowed 5&deg;,
	// then we arrived.
	if (angle <= maxAngle) 
	{
		return target;
	}

	float fT = maxAngle / angle;
	angle = maxAngle;

	glm::quat res = (sin((1.0f - fT) * angle) * current + sin(fT * angle) * target) / sin(angle);
	res = normalize(res);
	return res;
}

glm::quat Framework::Transform::CalculateRotationBetweenOrientations(glm::quat start, glm::quat end)
{
	return end * inverse(start);
}

// Stolen from http://www.opengl-tutorial.org/intermediate-tutorials/tutorial-17-quaternions/
glm::quat Framework::Transform::CalculateRotationBetweenOrientations(glm::vec3 start, glm::vec3 dest)
{
	assert(start != glm::vec3( 0.0f, 0.0f, 0.0f ) 
		&& dest != glm::vec3( 0.0f, 0.0f, 0.0f ));
	start = normalize(start);
	dest = normalize(dest);

	float cosTheta = dot(start, dest);
	glm::vec3 rotationAxis;

	if (cosTheta < -1 + 0.001f) 
	{
		// special case when vectors in opposite directions:
		// there is no "ideal" rotation axis
		// So guess one; any will do as long as it's perpendicular to start
		rotationAxis = cross(glm::vec3(0.0f, 0.0f, 1.0f), start);
		if (glm::length2(rotationAxis) < 0.01) // bad luck, they were parallel, try again!
		{
			rotationAxis = cross(glm::vec3(1.0f, 0.0f, 0.0f), start);
		}

		rotationAxis = normalize(rotationAxis);
		return glm::angleAxis(glm::radians(180.0f), rotationAxis);
	}

	rotationAxis = cross(start, dest);

	float s = sqrt((1 + cosTheta) * 2);
	float invs = 1 / s;

	return glm::quat(
		s * 0.5f,
		rotationAxis.x * invs,
		rotationAxis.y * invs,
		rotationAxis.z * invs
	);
}

// Stolen from https://stackoverflow.com/questions/44705398/about-glm-quaternion-rotation
glm::vec3 Framework::Transform::RotateVector(glm::vec3 v, glm::quat q)
{
	glm::vec3 quatAsVector = { q.x, q.y, q.z };
	return v * (q.w * q.w - glm::dot(quatAsVector, quatAsVector)) + 2.0f * quatAsVector * dot(quatAsVector, v) + 2.0f * q.w * cross(quatAsVector, v);
}

void Framework::Transform::Serialize(Framework::Data::Scope& parentScope) const
{
	Data::Scope& transformScope = parentScope.AddChild("Transform");

	if (mParent != nullptr)
	{
		const Entity* owner = mParent->GetOwner();

		if (owner != nullptr)
		{
			transformScope.AddVariable("parentOwnerId") << mParent->GetOwner()->GetId();
		}
	}
	
	transformScope.AddVariable("p") << mLocalPosition;
	transformScope.AddVariable("o") << mLocalOrientation;

	if (mLocalScale != glm::vec3{ 1.0f, 1.0f, 1.0f })
	{
		transformScope.AddVariable("s") << mLocalScale;
	}
}

void Framework::Transform::Deserialize(const Framework::Data::Scope& parentScope, Scene& scene)
{
	const Data::Scope& transformScope = parentScope.GetScope("Transform");

	std::optional<Data::Variable> parentOwnerId = transformScope.TryGetVariable("parentOwnerId");
	if (parentOwnerId.has_value())
	{
		EntityId tmp;
		parentOwnerId.value() >> tmp;
		scene.mEntityManager->DelayedIdRequest(tmp, 
			[](Entity& entity, void* me)
			{
				static_cast<Transform*>(me)->SetParent(&entity.GetTransform());
			}, this);
	}

	LoadFrom(transformScope);
}

void Framework::Transform::LoadFrom(const Framework::Data::Scope& scope)
{
	scope.GetVariable("p") >> mLocalPosition;
	scope.GetVariable("o") >> mLocalOrientation;

	std::optional<Data::Variable> scale = scope.TryGetVariable("s");
	if (scale.has_value())
	{
		scale.value() >> mLocalScale;
	}
	else
	{
		mLocalScale = { 1.0f, 1.0f, 1.0f };
	}
}

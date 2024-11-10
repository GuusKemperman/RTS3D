#pragma once

namespace Framework
{
	namespace Data
	{
		class Scope;
	}

	class Camera;
	class Entity;
	class Scene;

	class Transform :
		public btMotionState
	{
	public:
		Transform();
		Transform(Entity* owner);

		~Transform();

		Transform(const Transform& other);
		void operator=(const Transform& other);

		btTransform ToBullet() const;
		void getWorldTransform(btTransform& worldTrans) const override;
		void setWorldTransform(const btTransform& worldTrans) override;

		glm::mat4 GetLocalMatrix() const;
		glm::mat4 GetWorldMatrix() const;

		inline glm::vec3 GetLocalForward() const { return RotateVector(glm::vec3{ 0.0f, 0.0f, 1.0f }, mLocalOrientation); }
		inline glm::vec3 GetLocalUp() const { return RotateVector(glm::vec3{ 0.0f, 1.0f, 0.0f }, mLocalOrientation); }
		inline glm::vec3 GetLocalRight() const { return RotateVector(glm::vec3{ -1.0f, 0.0f, 0.0f }, mLocalOrientation); }

		inline glm::vec3 GetWorldForward() const { return RotateVector(glm::vec3{ 0.0f, 0.0f, 1.0f }, GetWorldOrientation()); }
		inline glm::vec3 GetWorldUp() const { return RotateVector(glm::vec3{ 0.0f, 1.0f, 0.0f }, GetWorldOrientation()); }
		inline glm::vec3 GetWorldRight() const { return RotateVector(glm::vec3{ -1.0f, 0.0f, 0.0f }, GetWorldOrientation()); }

		glm::vec2 GetLocalForward2D() const;

		void SetLocalForward(const glm::vec3 forward);
		void SetLocalUp(const glm::vec3 up);
		void SetLocalRight(const glm::vec3 right);

		void SetParent(Transform* parent);
		inline Transform* GetParent() const { return mParent; }

		inline Entity* GetOwner() const { return mOwner; }

		inline const std::vector<Transform*>& GetChildren() const { return mChildren; }

		inline bool IsOrphan() const { return mParent == nullptr; }

		inline glm::vec3 GetWorldPosition() const { return GetWorldMatrix() * glm::vec4{0.0f, 0.0f, 0.0f, 1.0f}; }
		glm::quat GetWorldOrientation() const;
		//inline glm::vec3 GetWorldScale() const { assert(true); }

		inline glm::vec3 GetLocalPosition() const { return mLocalPosition; }
		inline glm::vec2 GetLocalPosition2D() const { return { mLocalPosition.x, mLocalPosition.z }; }
		inline glm::quat GetLocalOrientation() const { return mLocalOrientation; }
		inline glm::vec3 GetLocalOrienationEuler() const { return glm::eulerAngles(mLocalOrientation); }
		inline glm::vec3 GetLocalScale() const { return mLocalScale; }

		inline void SetLocalPosition(const glm::vec3 position)	{ mLocalPosition = position; }
		inline void SetLocalPosition(const glm::vec2 position)	{ mLocalPosition.x = position.x, mLocalPosition.z = position.y; }

		// In radians
		inline void SetLocalOrientation(const glm::vec3 rotationEuler) { mLocalOrientation = glm::quat{ rotationEuler }; }
		inline void SetLocalOrientation(const glm::quat rotation) { mLocalOrientation = rotation; }

		inline void SetLocalScale(const glm::vec3 scale)		{ mLocalScale = scale; }

		inline void SetLocalPosition(const float x, const float y, const float z)	{ SetLocalPosition(glm::vec3{ x, y, z }); }
		inline void SetLocalOrientation(const float x, const float y, const float z)	{ SetLocalOrientation(glm::vec3{ x, y, z }); }
		inline void SetLocalScale(const float x, const float y, const float z)		{ SetLocalScale(glm::vec3{ x, y, z }); }

		void SetWorldOrientation(const glm::quat orientation);

		inline void TranslateLocalPosition(const float x, const float y, const float z) { SetLocalPosition(mLocalPosition + glm::vec3{ x, y, z }); }
		inline void TranslateLocalPosition(const glm::vec3 translation) { SetLocalPosition(mLocalPosition + translation); }
		inline void TranslateLocalPosition(const glm::vec2 translation) { SetLocalPosition(mLocalPosition.x + translation.x, mLocalPosition.y, mLocalPosition.z + translation.y); }

		static glm::quat CalculateOrientationTowards(glm::quat current, glm::quat target, const float maxAngle);
		static glm::quat CalculateRotationBetweenOrientations(glm::vec3 start, glm::vec3 dest);
		static glm::quat CalculateRotationBetweenOrientations(glm::quat start, glm::quat end);

		static glm::vec3 RotateVector(glm::vec3 v, glm::quat q);

		void Serialize(Framework::Data::Scope& parentScope) const;
		void Deserialize(const Framework::Data::Scope& parentScope, Scene& scene);

		void LoadFrom(const Framework::Data::Scope& scope);

		//static float CalculateAngleBetween(glm::quat quat1, glm::quat quat2);
		void Draw(Camera& camera) const;
	private:
		void AttachChild(Transform* child);
		void DetachChild(Transform* child);

		Entity* const mOwner{};

		Transform* mParent{};
		std::vector<Transform*> mChildren{};

		glm::vec3 mLocalPosition{};
		glm::quat mLocalOrientation = { 1.0f, 0.0f, 0.0f, 0.0f };
		glm::vec3 mLocalScale = { 1.0f, 1.0f, 1.0f };
	};
}
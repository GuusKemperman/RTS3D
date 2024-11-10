#pragma once
#include "Transform.h"
#include "Frustum.h"
#include "Inquirer.h"

namespace Framework
{
	namespace Data
	{
		class SavedData;
		class Scope;
	}

	class Scene;
	class Mesh;
	class MyShader;
	class BoundingBox2D;

	class Animator;

	class Camera
	{
	public:
		Camera(Scene& scene);
		~Camera();

		inline Transform& GetTransform() { return mTransform; }
		inline const Transform& GetTransform() const { return mTransform; }

		inline const btCollisionObject* GetFrustum() const { return mFrustumObject.get(); }

		inline float GetZoom() const { return Math::lerp(sMinZoom, sMaxZoom, mZoomPercentage); }
		inline float GetZoomPercentage() const { return mZoomPercentage; }
		inline void SetZoomPercentage(float zoom) { mZoomPercentage = std::clamp(zoom, 0.0f, 1.0f); }

		void DrawScene();

		static const Mesh* const GetMesh(ushort meshId);
		static MeshId GenerateMeshId(const Mesh* const mesh);
		
		void RequestInstanceDraw(const MeshId meshId, const glm::mat4& modelMatrix);

		void RequestDebugLineDraw(const glm::vec3 lineStart, const glm::vec3 lineEnd, const glm::vec3 color);
		void DrawBox(const BoundingBox2D& boxScreenSpace) const;
		void DrawLines(const std::vector<glm::vec3>& vertices, const std::vector<glm::vec3>& colors, const glm::mat4& MVP) const;

		inline const glm::mat4& GetViewProjection() const { return mViewProjection; }

		glm::vec3 CalculateRayDirection(const glm::ivec2 screenPos) const;

		void Serialize(Framework::Data::Scope& parentScope) const;
		void Deserialize(const Framework::Data::Scope& parentScope);

		static constexpr float zFar = 500.0f;
		static constexpr float zNear = 10.0f;

	private:
		float CalculateFOV() const;

		void ExecuteInstancingRequests();

		void UpdateFrustum();

		Scene& mScene;

		static constexpr float sMinZoom = 1.0f;
		static constexpr float sMaxZoom = 50.0f;
		float mZoomPercentage = 0.0f;

		static constexpr float sAspectRatio = static_cast<float>(sScreenWidth) / static_cast<float>(sScreenHeight);

		Transform mTransform{};

		FrustumShape mFrustumShape{};
		std::unique_ptr<btCollisionObject> mFrustumObject{};
		Inquirer mInquirer{};

		// Set to infinity to force the camera to update on the first frame.
		glm::vec3 mLastFramePosition = { INFINITY, INFINITY, INFINITY };
		glm::quat mLastFrameOrientation = { INFINITY, INFINITY, INFINITY, INFINITY };
		float mLastFrameZoom = INFINITY;

		glm::mat4 mView{};
		glm::mat4 mProjection{};
		glm::mat4 mViewProjection{};

		std::array<std::vector<glm::mat4>, 64> mInstancingRequests{};

		std::vector<glm::vec3> mLineRequestsVertexPosition{};
		std::vector<glm::vec3> mLineRequestsVertexColor{};

		std::shared_ptr<MyShader> mDebugShader{};
		GLuint mDebugVertexBuffer{};
		GLuint mDebugColorBuffer{};
		GLuint mDebugVertexArrayObject{};

		std::unique_ptr<Data::SavedData> mSettings{};
	};
}
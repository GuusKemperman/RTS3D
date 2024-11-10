#include "precomp.h"
#include "Camera.h"

#include "Scene.h"
#include "Terrain.h"
#include "AssetManager.h"
#include "Mesh.h"
#include "MyShader.h"
#include "Material.h"
#include "EntityManager.h"
#include "BoundingBox2D.h"
#include "Physics.h"
#include "SavedData.h"
#include "Scope.h"

#include "ImguiHelpers.h"
#include "InputManager.h"
#include "TimeManager.h"
#include "Terrain.h"
#include "game.h"
#include "TimeManager.h"

#ifdef DEBUG
constexpr bool sShowDebugWindow = true;
#else
constexpr bool sShowDebugWindow = false;
#endif // DEBUG


Framework::Camera::Camera(Scene& scene) :
	mScene(scene)
{
	mDebugShader = AssetManager::Inst().GetAsset<MyShader>("shaders/debugshader.vert,shaders/debugshader.frag");

	glGenBuffers(1, &mDebugVertexBuffer);
	glGenBuffers(1, &mDebugColorBuffer);
	glGenVertexArrays(1, &mDebugVertexArrayObject);

	mTransform.SetLocalPosition(50.0f, 50.0f, 10.0f);

	mInquirer.mCollisionObject.setCollisionShape(&mFrustumShape);
	mInquirer.mCollisionObject.setCollisionFlags(mInquirer.mCollisionObject.getCollisionFlags() | btCollisionObject::CF_NO_CONTACT_RESPONSE);
	mInquirer.m_collisionFilterGroup = Physics::Group::cameraGroup;
	mInquirer.m_collisionFilterMask = Physics::Mask::cameraMask;
	UpdateFrustum();

	mSettings = std::make_unique<Framework::Data::SavedData>("settings.txt");
}

Framework::Camera::~Camera()
{
	glDeleteBuffers(1, &mDebugVertexBuffer);
	glDeleteBuffers(1, &mDebugColorBuffer);
	glDeleteVertexArrays(1, &mDebugVertexArrayObject);
}

void Framework::Camera::DrawScene()
{
	static bool drawWireFrame = false;
	static bool drawMeshes = true;
	static bool drawBounds = false;
	static bool updateFrustum = true;

	if (sShowDebugWindow)
	{
		ImGui::SetNextWindowPos({ sScreenWidth - 500, 200 });
		ImGui::Begin("RenderInfo");

		float timeScale = TimeManager::GetTimeScale();
		if (ImGui::SliderFloat("Time scale", &timeScale, 0.0f, 1.0f))
		{
			TimeManager::SetTimeScale(timeScale);
		}

		ImGui::Checkbox("Draw wireframe", &drawWireFrame);
		ImGui::Checkbox("Draw meshes", &drawMeshes);
		ImGui::Checkbox("Draw bounds", &drawBounds);
		ImGui::Checkbox("Update frustum", &updateFrustum);

		if (ImGui::Button("Kill switch"))
		{
			mScene.mGame.Quit();
		}
	}


	// Camera matrix
	const glm::vec3 position = mTransform.GetLocalPosition();
	const glm::quat orientation = mTransform.GetLocalOrientation();

	if (mLastFrameZoom != mZoomPercentage
		|| mLastFramePosition != position
		|| mLastFrameOrientation != orientation)
	{
		mView = glm::lookAt(mTransform.GetLocalPosition(), mTransform.GetLocalPosition() + mTransform.GetLocalForward(), mTransform.GetLocalUp());
		mProjection = glm::perspective(CalculateFOV(), sAspectRatio, zNear, zFar);

		mViewProjection = mProjection * mView;

		if (updateFrustum)
		{
			UpdateFrustum();
		}

		mLastFrameZoom = mZoomPercentage;
		mLastFramePosition = position;
		mLastFrameOrientation = orientation;
	}

	mScene.mPhysics->SetDebugMode(drawWireFrame ? btIDebugDraw::DBG_DrawWireframe : btIDebugDraw::DBG_NoDebug);
	mScene.mPhysics->DebugDraw();

	if (drawMeshes)
	{
		bool frustumCulling;
		mSettings->GetVariable("CPU-based Culling") >> frustumCulling;

		if (frustumCulling)
		{
			mScene.mPhysics->Query(mInquirer, mTransform);

			for (const btCollisionObject* obj : mInquirer.mCollidedWith)
			{
				Entity* entity = static_cast<Entity*>(obj->getUserPointer());

				if (entity != nullptr)
				{
					entity->Draw();
				}
			}
		}
		else
		{
			mScene.mEntityManager->DrawEntities();
		}
	}

	if (drawBounds)
	{
		const glm::vec2 size = { mScene.mTerrain->GetWorldSizeX(), mScene.mTerrain->GetWorldSizeZ() };
		const glm::vec3 color = { 1.0f, 0.0f, 0.0f };
		RequestDebugLineDraw({ 0.0f, 0.0f, 0.0f }, { 0.0f, 100.0f, 0.0f }, color);
		RequestDebugLineDraw({ size.x, 0.0f, 0.0f }, { size.x, 100.0f, 0.0f }, color);
		RequestDebugLineDraw({ 0.0f, 0.0f, size.y }, { 0.0f, 100.0f, size.y }, color);
		RequestDebugLineDraw({ size.x, 0.0f, size.y }, { size.x, 100.0f, size.y }, color);
	}

	ExecuteInstancingRequests();
	CheckGL();

	if (sShowDebugWindow)
	{
		static float avgDeltaTime = TimeManager::GetRawDeltaTime();
		avgDeltaTime = avgDeltaTime * 0.995f + TimeManager::GetRawDeltaTime() * 0.005f;

		std::string output = "DeltaTime = " + std::to_string(avgDeltaTime);
		ImGui::Text(output.c_str());
		ImGui::End();
	}
}

void Framework::Camera::RequestDebugLineDraw(const glm::vec3 lineStart, const glm::vec3 lineEnd, const glm::vec3 color)
{
	mLineRequestsVertexPosition.push_back(lineStart);
	mLineRequestsVertexPosition.push_back(lineEnd);

	mLineRequestsVertexColor.push_back(color);
	mLineRequestsVertexColor.push_back(color);
}

void Framework::Camera::RequestInstanceDraw(const MeshId meshId, const glm::mat4& modelMatrix)
{
	mInstancingRequests[meshId].push_back(modelMatrix);
}

void Framework::Camera::DrawBox(const BoundingBox2D& boxScreenSpace) const
{
	constexpr float depth = .5f;

	std::vector<glm::vec3> vertices
	{
		{ boxScreenSpace.GetTopLeft(), depth }, { boxScreenSpace.GetTopRight(), depth },
		{ boxScreenSpace.GetTopRight(), depth }, { boxScreenSpace.GetBottomRight(), depth },
		{ boxScreenSpace.GetBottomRight(), depth }, { boxScreenSpace.GetBottomLeft(), depth },
		{ boxScreenSpace.GetBottomLeft(), depth }, { boxScreenSpace.GetTopLeft(), depth }
	};

	std::vector<glm::vec3> colors
	{
		{ 1.0f, 1.0f, 1.0f }, { 1.0f, 1.0f, 1.0f },
		{ 1.0f, 1.0f, 1.0f },{ 1.0f, 1.0f, 1.0f },
		{ 1.0f, 1.0f, 1.0f },{ 1.0f, 1.0f, 1.0f },
		{ 1.0f, 1.0f, 1.0f },{ 1.0f, 1.0f, 1.0f },
	};

	DrawLines(vertices, colors, glm::mat4{ 1.0f });
}

void Framework::Camera::DrawLines(const std::vector<glm::vec3>& vertices, const std::vector<glm::vec3>& colors, const glm::mat4& MVP) const
{
	if (vertices.empty())
	{
		return;
	}

	mDebugShader->Bind();

	glBindVertexArray(mDebugVertexArrayObject);

	glBindBuffer(GL_ARRAY_BUFFER, mDebugVertexBuffer);
	glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(glm::vec3), &vertices[0], GL_STATIC_DRAW);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), (void*)0);
	glEnableVertexAttribArray(0);

	glBindBuffer(GL_ARRAY_BUFFER, mDebugColorBuffer);
	glBufferData(GL_ARRAY_BUFFER, colors.size() * sizeof(glm::vec3), &colors[0], GL_STATIC_DRAW);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), (void*)0);
	glEnableVertexAttribArray(1);

	mDebugShader->SetInputMatrix("MVP", MVP);

	glDrawArrays(GL_LINES, 0, static_cast<GLsizei>(vertices.size()));

	glBindVertexArray(0);

	mDebugShader->Unbind();

	CheckGL();
}

static std::array<const Framework::Mesh*, 64> sLoadedMeshes;

const Framework::Mesh* const Framework::Camera::GetMesh(ushort meshId)
{
	return sLoadedMeshes[meshId];
}

ushort Framework::Camera::GenerateMeshId(const Mesh* const mesh)
{
	for (ushort i = 0u; i < sLoadedMeshes.size(); i++)
	{
		if (sLoadedMeshes[i] == nullptr)
		{
			sLoadedMeshes[i] = mesh;
			return i;
		}
	}

	assert(false && "No free space for loading meshes!");
	return 0;
}

// Borrowed from https://stackoverflow.com/questions/7692988/opengl-math-projecting-screen-space-to-world-space-coords
glm::vec3 Framework::Camera::CalculateRayDirection(const glm::ivec2 screenPos) const
{
	const glm::vec2 asFloat = screenPos;
	const glm::mat4 invMat = glm::inverse(mProjection * mView);
	const glm::vec4 nearVec = glm::vec4((asFloat.x - sHalfScreenWidth) / sHalfScreenWidth, -1 * (asFloat.y - sHalfScreenHeight) / sHalfScreenHeight, -1, 1.0);
	const glm::vec4 farVec = glm::vec4((asFloat.x - sHalfScreenWidth) / sHalfScreenWidth, -1 * (asFloat.y - sHalfScreenHeight) / sHalfScreenHeight, 1, 1.0);
	glm::vec4 nearResult = invMat * nearVec;
	glm::vec4 farResult = invMat * farVec;
	nearResult /= nearResult.w;
	farResult /= farResult.w;
	const glm::vec3 dir = glm::vec3(farResult - nearResult);
	return normalize(dir);
}

void Framework::Camera::Serialize(Framework::Data::Scope& parentScope) const
{
	Data::Scope& myScope = parentScope.AddChild("Camera");

	myScope.AddVariable("zoom") << mZoomPercentage;
	mTransform.Serialize(myScope);
}

void Framework::Camera::Deserialize(const Framework::Data::Scope& parentScope)
{
	std::optional<const Data::Scope*> myOptionalScope = parentScope.TryGetScope("Camera");
	if (!myOptionalScope.has_value())
	{
		LOGWARNING("Could not load in camera from file, data missing");
		return;
	}
	const Data::Scope& myScope = *myOptionalScope.value();

	myScope.GetVariable("zoom") >> mZoomPercentage;
	mTransform.Deserialize(myScope, mScene);
}

float Framework::Camera::CalculateFOV() const
{
	return glm::radians(45.0f / Math::lerp(sMinZoom, sMaxZoom, mZoomPercentage));
}

void Framework::Camera::ExecuteInstancingRequests()
{
	uint totalAmountOfObjectsDrawn = 0;
	uint amountOfRenderCallsMade = 0;

	for (ushort i = 0u; i < sLoadedMeshes.size(); i++)
	{
		const Mesh* mesh = sLoadedMeshes[i];

		if (mesh == nullptr)
		{
			break;
		}

		std::vector<glm::mat4>& requests = mInstancingRequests[i];

		if (requests.empty())
		{
			continue;
		}

		totalAmountOfObjectsDrawn += static_cast<uint>(requests.size());
		amountOfRenderCallsMade++;
		mesh->DrawInstances(*this, requests);

		requests.clear();
	}

	uint amountOfLinesToDraw = static_cast<uint>(mLineRequestsVertexPosition.size() / 2);

	DrawLines(mLineRequestsVertexPosition, mLineRequestsVertexColor, mViewProjection);
	mLineRequestsVertexPosition.clear();
	mLineRequestsVertexColor.clear();

	if (sShowDebugWindow)
	{
		std::string output = "ObjectsDrawn = " + std::to_string(totalAmountOfObjectsDrawn) +
			"\nDrawcalls = " + std::to_string(amountOfRenderCallsMade) +
			"\nAmountOfLines = " + std::to_string(amountOfLinesToDraw);
		ImGui::Text(output.c_str());
	}
}

void Framework::Camera::UpdateFrustum()
{
	if (mLastFrameZoom != mZoomPercentage)
	{
		mFrustumShape = { static_cast<float>(sScreenWidth), static_cast<float>(sScreenHeight), CalculateFOV(), zNear, zFar };
	}
	mInquirer.mCollisionObject.setWorldTransform(mTransform.ToBullet());
}
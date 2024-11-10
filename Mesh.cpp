#include "precomp.h"
#include "Mesh.h"

#include "AssetManager.h"
#include "Texture.h"
#include "MyShader.h"
#include "Material.h"
#include "Camera.h"

Framework::Mesh::Mesh()
{
	glGenVertexArrays(1, &mVertexArrayObject);

	glGenBuffers(1, &mInstancesBuffer);
	glGenBuffers(1, &mVertexBuffer);
	glGenBuffers(1, &mNormalBuffer);
	glGenBuffers(1, &mUVBuffer);
	glGenBuffers(1, &mTrianglesBuffer);
}

Framework::Mesh::Mesh(const std::string& filePath) :
	Mesh()
{
	mShader = AssetManager::Inst().GetAsset<MyShader>("shaders/standard.vert,shaders/standard.frag");
	mMeshId = Camera::GenerateMeshId(this);

	Assimp::Importer importer{};
	LoadFrom(filePath, importer.ReadFile(filePath, sReadFileFlags));
}

Framework::Mesh::~Mesh()
{
	glDeleteBuffers(1, &mInstancesBuffer);
	glDeleteBuffers(1, &mVertexBuffer);
	glDeleteBuffers(1, &mNormalBuffer);
	glDeleteBuffers(1, &mUVBuffer);
	glDeleteBuffers(1, &mTrianglesBuffer);

	glDeleteVertexArrays(1, &mVertexArrayObject);
	CheckGL();
}

void Framework::Mesh::SetVertices(std::vector<glm::vec3> vertices)
{
	mVertices = std::move(vertices);

	glBindVertexArray(mVertexArrayObject);
	glBindBuffer(GL_ARRAY_BUFFER, mVertexBuffer);

	glBufferData(GL_ARRAY_BUFFER, mVertices.size() * sizeof(glm::vec3), &mVertices[0], GL_STATIC_DRAW);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), (void*)0);
	glEnableVertexAttribArray(0);

	glBindVertexArray(0);

	UpdateRadius();
}

void Framework::Mesh::SetNormals(std::vector<glm::vec3> normals)
{
	mNormals = std::move(normals);

	glBindVertexArray(mVertexArrayObject);
	glBindBuffer(GL_ARRAY_BUFFER, mNormalBuffer);

	glBufferData(GL_ARRAY_BUFFER, mNormals.size() * sizeof(glm::vec3), &mNormals[0], GL_STATIC_DRAW);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), (void*)0);
	glEnableVertexAttribArray(1);

	glBindVertexArray(0);
}

void Framework::Mesh::SetUVs(std::vector<glm::vec2> UVs)
{
	mUVs = std::move(UVs);

	glBindVertexArray(mVertexArrayObject);
	glBindBuffer(GL_ARRAY_BUFFER, mUVBuffer);

	glBufferData(GL_ARRAY_BUFFER, mUVs.size() * sizeof(glm::vec2), &mUVs[0], GL_STATIC_DRAW);
	glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(glm::vec2), (void*)0);
	glEnableVertexAttribArray(2);

	glBindVertexArray(0);
}

void Framework::Mesh::SetTriangles(std::vector<Triangle> triangles)
{
	mTriangles = std::move(triangles);

	glBindVertexArray(mVertexArrayObject);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mTrianglesBuffer);

	glBufferData(GL_ELEMENT_ARRAY_BUFFER, mTriangles.size() * sizeof(Triangle), &mTriangles[0], GL_STATIC_DRAW);

	glBindVertexArray(0);
}

void Framework::Mesh::SetMaterial(const std::shared_ptr<Material>& material)
{
	mMaterial = material;
}

void Framework::Mesh::SetShader(const std::shared_ptr<MyShader>& shader)
{
	mShader = shader;
}

void Framework::Mesh::DrawInstances(const Camera& camera, const std::vector<glm::mat4>& instances) const
{
	glBindVertexArray(mVertexArrayObject);

	glBindBuffer(GL_ARRAY_BUFFER, mInstancesBuffer);
	glBufferData(GL_ARRAY_BUFFER, instances.size() * sizeof(glm::mat4), &instances[0], GL_STATIC_DRAW);

	glEnableVertexAttribArray(3);
	glVertexAttribPointer(3, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(glm::vec4), (void*)0);
	glEnableVertexAttribArray(4);
	glVertexAttribPointer(4, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(glm::vec4), (void*)(1 * sizeof(glm::vec4)));
	glEnableVertexAttribArray(5);
	glVertexAttribPointer(5, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(glm::vec4), (void*)(2 * sizeof(glm::vec4)));
	glEnableVertexAttribArray(6);
	glVertexAttribPointer(6, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(glm::vec4), (void*)(3 * sizeof(glm::vec4)));

	glVertexAttribDivisor(3, 1);
	glVertexAttribDivisor(4, 1);
	glVertexAttribDivisor(5, 1);
	glVertexAttribDivisor(6, 1);

	mShader->Bind();

	mShader->SetInputTexture(0, "sampler", *mMaterial->GetDiffuse());
	mShader->SetInputMatrix("viewProjection", camera.GetViewProjection());
	mShader->SetFloat3("cameraPos", camera.GetTransform().GetLocalPosition());
	 
	glDrawElementsInstanced(GL_TRIANGLES, static_cast<GLsizei>(mTriangles.size() * 3u), GL_UNSIGNED_SHORT, 0, static_cast<GLsizei>(instances.size()));

	mShader->Unbind();
	
	glBindVertexArray(0);
	CheckGL();
}

void Framework::Mesh::SimpleDraw() const
{
	glBindVertexArray(mVertexArrayObject);
	
	glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(mTriangles.size() * 3u), GL_UNSIGNED_SHORT, 0);

	glBindVertexArray(0);
	CheckGL();
}

void Framework::Mesh::LoadFrom(const std::string& filePath, const aiScene* scene)
{
	assert(scene != nullptr
		&& scene->mNumMeshes == 1);

	const aiMesh* mesh = scene->mMeshes[0];

	assert(mesh->mNumVertices < std::numeric_limits<ushort>::max());

	std::vector<glm::vec3> vertices{};
	std::vector<glm::vec3> normals{};
	std::vector<glm::vec2> uvs{};

	for (size_t i = 0; i < mesh->mNumVertices; i++)
	{
		//const aiVector3D& vertex = mesh->mVertices[i];
		const aiVector3D& vertex = mesh->mVertices[i];
		//assert(vertex == withMatrixApplied);
		vertices.emplace_back(vertex.x, vertex.y, vertex.z);

		const aiVector3D& normal = mesh->mNormals[i];
		normals.emplace_back(normal.x, normal.y, normal.z);
		if (mesh->mTextureCoords[0])
		{
			const aiVector3D& uv = mesh->mTextureCoords[0][i];
			uvs.emplace_back(uv.x, uv.y);
		}
		else
		{
			uvs.emplace_back(0.0f, 0.0f);
		}
	}
	SetVertices(std::move(vertices));
	SetNormals(std::move(normals));
	SetUVs(std::move(uvs));

	std::vector<Triangle> triangles{};
	for (size_t i = 0; i < mesh->mNumFaces; i++)
	{
		const aiFace& face = mesh->mFaces[i];
		assert(face.mNumIndices == 3);

		triangles.emplace_back(static_cast<GLushort>(face.mIndices[0]),
			static_cast<GLushort>(face.mIndices[1]),
			static_cast<GLushort>(face.mIndices[2]));
	}

	SetTriangles(std::move(triangles));

	// Not const, I get errors for trying to retrieve the name when building for the pi. This is easier than figuring out why that happens.
	aiMaterial* aiMaterial = scene->mMaterials[mesh->mMaterialIndex];

	const size_t lastSlash = filePath.find_last_of('/');
	const std::string materialDirectory = filePath.substr(sAssetsRoot.size(), lastSlash + 1 - sAssetsRoot.size());

	const std::string materialKey = aiMaterial->GetName().C_Str();
	mMaterial = AssetManager::Inst().GetAsset<Material>(materialKey);

	if (!mMaterial->HasBeenLoaded())
	{
		mMaterial->LoadFrom(aiMaterial, materialDirectory);
	}

	CheckGL();
}

void Framework::Mesh::UpdateRadius()
{
	for (const glm::vec3& vertex : mVertices)
	{
		mRadius = std::max(mRadius, glm::length(vertex));
	}
}
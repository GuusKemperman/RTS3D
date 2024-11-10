#pragma once

namespace Framework
{
	class Camera;
	class MyShader;
	class Material;

	class Mesh
	{
	public:
		Mesh();
		Mesh(const std::string& filePath);
		virtual ~Mesh();

		inline size_t GetNumOfVertices() const { return mVertices.size(); }
		inline GLuint GetVertexArrayObject() const { return mVertexArrayObject; }
		inline const float GetRadius() const { return mRadius; }
		inline MeshId GetMeshId() const { return mMeshId; }
		inline const MyShader* const GetShader() const { return mShader.get(); }
		inline const Material* const GetMaterial() const { return mMaterial.get(); }

		struct Triangle
		{
			Triangle() = default;
			Triangle(GLushort a, GLushort b, GLushort c) { cell[0] = a, cell[1] = b, cell[2] = c; }
			Triangle(int a, int b, int c) { cell[0] = static_cast<GLushort>(a), cell[1] = static_cast<GLushort>(b), cell[2] = static_cast<GLushort>(c); }

			GLushort cell[3]{};
		};

		void SetVertices(std::vector<glm::vec3> vertices);
		void SetNormals(std::vector<glm::vec3> normals);
		void SetUVs(std::vector<glm::vec2> UVs);
		void SetTriangles(std::vector<Triangle> triangles);

		void SetMaterial(const std::shared_ptr<Material>& material);
		void SetShader(const std::shared_ptr<MyShader>& shader);
		
		void DrawInstances(const Camera& camera, const std::vector<glm::mat4>& instancesMVPs) const;

		// Assumes that all the variables have already been set, including binding the shader.
		void SimpleDraw() const;


	protected:
		static constexpr uint sReadFileFlags = aiProcess_Triangulate | aiProcess_JoinIdenticalVertices;
		virtual void LoadFrom(const std::string& filePath, const aiScene* scene);

	private:
		void UpdateRadius();

		MeshId mMeshId{};

		std::vector<glm::vec3> mVertices{};
		std::vector<glm::vec2> mUVs{};
		std::vector<glm::vec3> mNormals{};
		std::vector<Triangle> mTriangles{};

		std::shared_ptr<Material> mMaterial{};
		std::shared_ptr<MyShader> mShader{};

		GLuint mVertexArrayObject{};
		GLuint mInstancesBuffer{};
		GLuint mVertexBuffer{};
		GLuint mNormalBuffer{};
		GLuint mUVBuffer{};
		GLuint mTrianglesBuffer{};

		float mRadius{};
	};
}
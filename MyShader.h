#pragma once

namespace Framework
{
	class Texture;

	class MyShader
	{
	public:
		// Paths seperated by a comma, e.g. assets/vertexpath.vert,assets/fragpath.frag
		MyShader(const std::string& vertexPathAndFragmentPath);
		~MyShader();

		void Bind() const;
		void Unbind() const;
		
		void SetInputTexture(const uint slot, const char* name, const Texture& texture) const;
		void SetInputMatrix(const char* name, const glm::mat4& matrix) const;
		void SetFloat(const char* name, const float v) const;
		void SetInt(const char* name, const int v) const;
		void SetUInt(const char* name, const uint v) const;
		void SetFloat3(const char* name, const glm::vec3& v) const;

	private:
		void Compile(const char* vtext, const char* ftext);

		uint mId{};
	};
}
#include "precomp.h"
#include "MyShader.h"

#include "Texture.h"

#include "AssetManager.h"

Framework::MyShader::MyShader(const std::string& vertexPathAndFragmentPath)
{
	std::vector<std::string> paths = StringFunctions::SplitString(vertexPathAndFragmentPath.substr(sAssetsRoot.size()), ",");
	assert(paths.size() == 2);

	std::string vsText = StringFunctions::ReadFile((sAssetsRoot + paths[0]).c_str());
	std::string fsText = StringFunctions::ReadFile((sAssetsRoot + paths[1]).c_str());

	assert(!vsText.empty() 
		&& !fsText.empty());

	const char* vertexText = vsText.c_str();
	const char* fragmentText = fsText.c_str();
	Compile(vertexText, fragmentText);
}

Framework::MyShader::~MyShader()
{
	glDeleteProgram(mId);
}

GLuint LoadShader(GLenum type, const char* shaderSrc)
{
	// 1st create the shader object
	GLuint TheShader = glCreateShader(type);

	if (TheShader == 0)
	{
		CheckGL();
		assert(false);
	}

	// pass the shader source then compile it
	glShaderSource(TheShader, 1, &shaderSrc, NULL);
	glCompileShader(TheShader);

	GLint  IsItCompiled;

	// After the compile we need to check the status and report any errors
	glGetShaderiv(TheShader, GL_COMPILE_STATUS, &IsItCompiled);
	if (!IsItCompiled)
	{
		GLint RetinfoLen = 0;
		glGetShaderiv(TheShader, GL_INFO_LOG_LENGTH, &RetinfoLen);
		if (RetinfoLen > 1)
		{
			// standard output for errors
			char* infoLog = (char*)malloc(sizeof(char) * RetinfoLen);
			glGetShaderInfoLog(TheShader, RetinfoLen, NULL, infoLog);
			fprintf(stderr, "Error compiling this shader:\n%s\n", infoLog);
			assert(false && infoLog);
			free(infoLog);
		}
		glDeleteShader(TheShader);
		assert(false);
	}
	return TheShader;
}

void Framework::MyShader::Compile(const char* vtext, const char* ftext)
{
	GLuint vertexId = LoadShader(GL_VERTEX_SHADER, vtext);;
	GLuint fragId = LoadShader(GL_FRAGMENT_SHADER, ftext);;

	mId = glCreateProgram();
	glAttachShader(mId, vertexId);
	glAttachShader(mId, fragId);
	glLinkProgram(mId);

	glDeleteShader(vertexId);
	glDeleteShader(fragId);
	CheckGL();
}

void Framework::MyShader::Bind() const
{
	CheckGL();
	glUseProgram(mId);
	CheckGL();
}

void Framework::MyShader::Unbind() const
{
	glUseProgram(0);
	CheckGL();
}

void Framework::MyShader::SetInputTexture(const uint slot, const char* name, const Texture& texture) const
{
	glActiveTexture(GL_TEXTURE0 + slot);
	glBindTexture(GL_TEXTURE_2D, texture.GetId());
	glUniform1i(glGetUniformLocation(mId, name), slot);
	CheckGL();
}

void Framework::MyShader::SetInputMatrix(const char* name, const glm::mat4& matrix) const
{
	glUniformMatrix4fv(glGetUniformLocation(mId, name), 1, GL_FALSE, &matrix[0][0]);
	CheckGL();
}

void Framework::MyShader::SetFloat(const char* name, const float v) const
{
	glUniform1f(glGetUniformLocation(mId, name), v);
	CheckGL();
}

void Framework::MyShader::SetInt(const char* name, const int v) const
{
	glUniform1i(glGetUniformLocation(mId, name), v);
	CheckGL();
}

void Framework::MyShader::SetUInt(const char* name, const uint v) const
{
	glUniform1ui(glGetUniformLocation(mId, name), v);
	CheckGL();
}

void Framework::MyShader::SetFloat3(const char* name, const glm::vec3& v) const
{
	glUniform3fv(glGetUniformLocation(mId, name), 1, &v[0]);
	CheckGL();
}


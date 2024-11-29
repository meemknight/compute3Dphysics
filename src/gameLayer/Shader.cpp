#include "shader.h"
#include <string>
#include <fstream>
#include <iostream>

char *readEntireFile(const char *source)
{
	std::ifstream file;
	file.open(source);

	if (!file.is_open())
	{
		std::cout << "Error openning file: " << source << "\n";
		return nullptr;
	}

	GLint size = 0;
	file.seekg(0, file.end);
	size = file.tellg();
	file.seekg(0, file.beg);

	char *fileContent = new char[size + 1] {};

	file.read(fileContent, size);

	file.close();

	return fileContent;
}

bool Shader::loadShaderProgramFromFile(const char *vertexShaderPath,
	const char *fragmentShaderPath)
{

	char *vertexData = readEntireFile(vertexShaderPath);
	char *fragmentData = readEntireFile(fragmentShaderPath);


	if (vertexData == nullptr || fragmentData == nullptr)
	{
		delete[] vertexData;
		delete[] fragmentData;

		return 0;
	}

	bool rez = loadShaderProgramFromData(vertexData, fragmentData);


	delete[] vertexData;
	delete[] fragmentData;

	return rez;
}


GLint createShaderFromData(const char *data, GLenum shaderType)
{
	GLuint shaderId = glCreateShader(shaderType);
	glShaderSource(shaderId, 1, &data, nullptr);
	glCompileShader(shaderId);

	GLint rezult = 0;
	glGetShaderiv(shaderId, GL_COMPILE_STATUS, &rezult);

	if (!rezult)
	{
		char *message = 0;
		int   l = 0;

		glGetShaderiv(shaderId, GL_INFO_LOG_LENGTH, &l);

		if (l)
		{
			message = new char[l];

			glGetShaderInfoLog(shaderId, l, &l, message);

			message[l - 1] = 0;

			std::cout << data << ":\n" << message << "\n";

			delete[] message;

		}
		else
		{
			std::cout << data << ":\n" << "unknown error" << "\n";
		}

		glDeleteShader(shaderId);

		shaderId = 0;
		return shaderId;
	}

	return shaderId;

}


bool Shader::loadShaderProgramFromData(const char *vertexShaderData,
	const char *fragmentShaderData)
{
	auto vertexId = createShaderFromData(vertexShaderData, GL_VERTEX_SHADER);

	if (vertexId == 0) { return 0; }

	auto fragmentId = createShaderFromData(fragmentShaderData, GL_FRAGMENT_SHADER);

	if (fragmentId == 0)
	{
		glDeleteShader(vertexId);
		return 0;
	}

	id = glCreateProgram();

	glAttachShader(id, vertexId);
	glAttachShader(id, fragmentId);

	glLinkProgram(id);

	glDeleteShader(vertexId);
	glDeleteShader(fragmentId);

	GLint info = 0;
	glGetProgramiv(id, GL_LINK_STATUS, &info);

	if (info != GL_TRUE)
	{
		char *message = 0;
		int   l = 0;

		glGetProgramiv(id, GL_INFO_LOG_LENGTH, &l);

		message = new char[l];

		glGetProgramInfoLog(id, l, &l, message);

		message[l - 1] = 0;

		std::cout << "Link error: " << message << "\n";

		delete[] message;

		glDeleteProgram(id);
		id = 0;
		return 0;
	}

	glValidateProgram(id);

	return true;
}

bool Shader::loadComputeShaderProgramFromFile(const char *computeShaderData)
{
	char *computeData = readEntireFile(computeShaderData);

	if (computeData == nullptr)
	{
		delete[] computeData;
		return 0;
	}

	bool rez = loadComputeShaderProgramFromData(computeData);


	delete[] computeData;

	return rez;
}

bool Shader::loadComputeShaderProgramFromData(const char *computeShaderData)
{
	GLuint shaderId = glCreateShader(GL_COMPUTE_SHADER);
	glShaderSource(shaderId, 1, &computeShaderData, nullptr);
	glCompileShader(shaderId);

	GLint rezult = 0;
	glGetShaderiv(shaderId, GL_COMPILE_STATUS, &rezult);

	if (!rezult)
	{
		char *message = 0;
		int   l = 0;

		glGetShaderiv(shaderId, GL_INFO_LOG_LENGTH, &l);

		if (l)
		{
			message = new char[l];

			glGetShaderInfoLog(shaderId, l, &l, message);

			message[l - 1] = 0;

			std::cout << computeShaderData << ":\n" << message << "\n";

			delete[] message;

		}
		else
		{
			std::cout << computeShaderData << ":\n" << "unknown error" << "\n";
		}

		glDeleteShader(shaderId);

		shaderId = 0;
		return shaderId;
	}

	id = glCreateProgram();
	glAttachShader(id, shaderId);
	glLinkProgram(id);
	glValidateProgram(id);

	return shaderId;

}

void Shader::bind()
{
	glUseProgram(id);
}

void Shader::clear()
{
	glDeleteProgram(id);

	*this = {};
}

GLint Shader::getUniform(const char *name)
{
	GLint uniform = glGetUniformLocation(id, name);
	if (uniform == -1)
	{
		std::cout << "error uniform: " << name << "\n";
	}
	return uniform;
}

#pragma once
#include <glad/glad.h>


struct Shader
{
	GLuint id = 0;

	bool loadShaderProgramFromFile(const char *vertexShaderPath, const char *fragmentShaderPath);
	
	bool loadShaderProgramFromData(const char *vertexShaderData, const char *fragmentShaderData);

	bool loadComputeShaderProgramFromFile(const char *computeShaderPath);
	bool loadComputeShaderProgramFromData(const char *computeShaderData);


	void bind();

	void clear();

	GLint getUniform(const char *name);
};

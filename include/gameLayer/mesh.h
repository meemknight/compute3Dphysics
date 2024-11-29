#pragma once

#include <glad/glad.h>


class Mesh
{
public:

	GLuint vao = 0;
	GLuint vbo = 0;
	GLuint ibo = 0;
	int vertexCount = 0;

	void loadFromData(float *data, int size, unsigned short *indices, int indicesSize);
};

#pragma once
#include <mesh.h>
#include <vector>
#include <string>

class MeshAndData
{
public:

	Mesh mesh;

	//todo materials and textures

	std::string name;

};




class Model
{
public:

	std::string name;

	std::vector<MeshAndData> meshes;

	void loadFromFile(const char *file);
};
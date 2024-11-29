#include "model.h"

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <iostream>


void Model::loadFromFile(const char *file)
{

	*this = {}; //clear the model

	Assimp::Importer importer;

	//keep only triangles from the mesh
	importer.SetPropertyInteger(AI_CONFIG_PP_SBP_REMOVE, aiPrimitiveType_LINE | aiPrimitiveType_POINT);
	importer.SetPropertyInteger(AI_CONFIG_PP_RVC_FLAGS, aiComponent_NORMALS);


	//read the file
	{


		const aiScene *scene = importer.ReadFile(file, 
			aiProcess_Triangulate | aiProcess_GenUVCoords | aiProcess_GenNormals);

		if (scene)
		{


			//avem mai multe noduri in shcena
			for (unsigned int i = 0; i < scene->mRootNode->mNumChildren; ++i)
			{

				const aiNode *node = scene->mRootNode->mChildren[i];




				//fiecare nod are mai multe meshuri
				for (int m = 0; m < node->mNumMeshes; m++)
				{
					const aiMesh *mesh = scene->mMeshes[node->mMeshes[m]];

					MeshAndData meshAndData;

					//load

					std::vector<float> vertices;
					vertices.reserve(mesh->mNumVertices * 3);

					for (unsigned int v = 0; v < mesh->mNumVertices; ++v)
					{
						vertices.push_back(mesh->mVertices[v].x);
						vertices.push_back(mesh->mVertices[v].y);
						vertices.push_back(mesh->mVertices[v].z);

						vertices.push_back(mesh->mNormals[v].x);
						vertices.push_back(mesh->mNormals[v].y);
						vertices.push_back(mesh->mNormals[v].z);
					}


					std::vector<unsigned short> indices;
					indices.reserve(mesh->mNumFaces * 3);

					for (unsigned int f = 0; f < mesh->mNumFaces; ++f)
					{
						const aiFace &face = mesh->mFaces[f];
						for (unsigned int i = 0; i < face.mNumIndices; ++i)
						{
							indices.push_back(face.mIndices[i]);
						}
					}


					meshAndData.mesh.loadFromData(vertices.data(), vertices.size() * sizeof(float),
							indices.data(), indices.size() * sizeof(unsigned short));

					meshes.push_back(meshAndData);

				}


			}


		}
		else
		{
			std::cout << "Error loading: " << file << "\n";
		}

	}

	//clears everything
	importer.FreeScene();


}

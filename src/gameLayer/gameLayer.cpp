#define GLM_ENABLE_EXPERIMENTAL
#include "gameLayer.h"
#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtx/transform.hpp>
#include "platformInput.h"
#include "imgui.h"
#include <iostream>
#include <sstream>
#include <platformTools.h>
#include <shader.h>
#include "mesh.h"
#include <camera.h>
#include <imfilebrowser.h>
#include <model.h>
#include <physics.h>
#include <profiler.h>

#define GPU_ENGINE 1
extern "C"
{
	__declspec(dllexport) unsigned long NvOptimusEnablement = GPU_ENGINE;
	__declspec(dllexport) int AmdPowerXpressRequestHighPerformance = GPU_ENGINE;
}

Shader shader;
Shader compute;
GLint computeu_Size;
GLint computeu_deltaTime;


Camera camera;

GLint viewProj;
GLint color;
GLint positions;
GLint size;


Shader instancedShader;
GLint instancedViewProj;
GLint instancedColor;



ImGui::FileBrowser fileBrowser;

Model cubeModel;
Model ballModel;
Model cilindruModel;

Simulator simulator;


Profiler cpuProfiler;
Profiler gpuProfiler;

auto getRandomNumber = [&](float min, float max)
{
	return (rand() % 2000 / 2000.f) * (max - min) + min;
};


int currentShaderReadBuffer = 0;
GLuint ssbo[2];


GLuint spheresSSBO;
GLuint cubesSSBO;
GLuint cylindresSSBO;

GLuint counters[3];

GLuint sphereVAO = 0;
GLuint cubeVAO = 0;
GLuint cylinderVAO = 0;

GLuint setupMeshVao(Mesh &mesh, GLint ssbo)
{
	GLuint vaoReturn = 0;

	glGenVertexArrays(1, &vaoReturn);

	glBindVertexArray(vaoReturn);


	glBindBuffer(GL_ARRAY_BUFFER, mesh.vbo);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, 0, 6 * sizeof(float), 0);
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 3, GL_FLOAT, 0, 6 * sizeof(float), (void *)(3 * sizeof(float)));

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh.ibo);


	glBindBuffer(GL_ARRAY_BUFFER, ssbo);
	glEnableVertexAttribArray(2);
	glVertexAttribPointer(2, 4, GL_FLOAT, 0, 8 * sizeof(float), 0);
	glVertexAttribDivisor(2, 1);
	glEnableVertexAttribArray(3);
	glVertexAttribPointer(3, 4, GL_FLOAT, 0, 8 * sizeof(float), (void *)(4 * sizeof(float)));
	glVertexAttribDivisor(3, 1);


	glBindVertexArray(0);

	return vaoReturn;
}

int spheresSize = 0;
int cubesSize = 0;
int cylindresSize = 0;

void uploadDataToGPU()
{
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo[currentShaderReadBuffer]);
	glBufferData(GL_SHADER_STORAGE_BUFFER, simulator.bodies.size()
		* sizeof(simulator.bodies[0]) ,simulator.bodies.data(), GL_DYNAMIC_READ);

	glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo[!currentShaderReadBuffer]);
	glBufferData(GL_SHADER_STORAGE_BUFFER, simulator.bodies.size()
		* sizeof(simulator.bodies[0]), simulator.bodies.data(), GL_DYNAMIC_READ);

	//determine sizes
	{
		spheresSize = 0;
		cubesSize = 0;
		cylindresSize = 0;

		for (auto &o : simulator.bodies)
		{
			if (o.type == TYPE_CIRCLE)spheresSize++;
			if (o.type == TYPE_BOX)cubesSize++;
			if (o.type == TYPE_CILINDRU)cylindresSize++;
		}

		glBindBuffer(GL_SHADER_STORAGE_BUFFER, spheresSSBO);
		glBufferData(GL_SHADER_STORAGE_BUFFER, spheresSize * sizeof(glm::vec4) * 2, 0, GL_DYNAMIC_READ);

		glBindBuffer(GL_SHADER_STORAGE_BUFFER, cubesSSBO);
		glBufferData(GL_SHADER_STORAGE_BUFFER, cubesSize * sizeof(glm::vec4) * 2, 0, GL_DYNAMIC_READ);

		glBindBuffer(GL_SHADER_STORAGE_BUFFER, cylindresSSBO);
		glBufferData(GL_SHADER_STORAGE_BUFFER, cylindresSize * sizeof(glm::vec4) * 2, 0, GL_DYNAMIC_READ);

	}



}

void bindDataToGPU() 
{
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, ssbo[currentShaderReadBuffer]);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, ssbo[!currentShaderReadBuffer]);

	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, spheresSSBO);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, cubesSSBO);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 4, cylindresSSBO);


	GLuint initialValue = 0;
	glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, counters[0]);
	glBufferData(GL_ATOMIC_COUNTER_BUFFER, sizeof(GLuint), &initialValue, GL_DYNAMIC_DRAW);

	glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, counters[1]);
	glBufferData(GL_ATOMIC_COUNTER_BUFFER, sizeof(GLuint), &initialValue, GL_DYNAMIC_DRAW);

	glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, counters[2]);
	glBufferData(GL_ATOMIC_COUNTER_BUFFER, sizeof(GLuint), &initialValue, GL_DYNAMIC_DRAW);

	glBindBufferBase(GL_ATOMIC_COUNTER_BUFFER, 5, counters[0]);  // Binding point 5
	glBindBufferBase(GL_ATOMIC_COUNTER_BUFFER, 6, counters[1]);  // Binding point 6
	glBindBufferBase(GL_ATOMIC_COUNTER_BUFFER, 7, counters[2]);  // Binding point 7

};

void readDataFromGPU()
{

	glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

	
	if (1)
	{


		//if (currentShaderReadBuffer)return;

		glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo[!currentShaderReadBuffer]);

		// Ensure GPU writes are complete
		//glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

		gpuProfiler.startSubProfile("Read GPU data");
		cpuProfiler.startSubProfile("Read GPU data");
		glGetBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, simulator.bodies.size() * sizeof(simulator.bodies[0]),
			simulator.bodies.data());
		cpuProfiler.endSubProfile("Read GPU data");
		gpuProfiler.endSubProfile("Read GPU data");
	}
	else
	{

		int spheresSize = 0;
		int cubesSize = 0;
		int cylindresSize = 0;

		for (auto &o : simulator.bodies)
		{
			if (o.type == TYPE_CIRCLE)spheresSize++;
			if (o.type == TYPE_BOX)cubesSize++;
			if (o.type == TYPE_CILINDRU)cylindresSize++;
		}

		std::vector<glm::vec4> spheresPositions;
		spheresPositions.resize(spheresSize * 2);

		std::vector<glm::vec4> cubesPositions;
		cubesPositions.resize(cubesSize * 2);

		std::vector<glm::vec4> cylindrePositions;
		cylindrePositions.resize(cylindresSize * 2);

		if (spheresSize)
		{
			glBindBuffer(GL_SHADER_STORAGE_BUFFER, spheresSSBO);
			glGetBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, spheresSize * sizeof(glm::vec4) * 2,
				spheresPositions.data());
		}

		if (cubesSize)
		{
			glBindBuffer(GL_SHADER_STORAGE_BUFFER, cubesSSBO);
			glGetBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, cubesSize * sizeof(glm::vec4) * 2,
				cubesPositions.data());
		}

		if (cylindresSize)
		{
			glBindBuffer(GL_SHADER_STORAGE_BUFFER, cylindresSSBO);
			glGetBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, cylindresSize * sizeof(glm::vec4) * 2,
				cylindrePositions.data());
		}

		int spheresCounter = 0;
		int cubesCounter = 0;
		int cylindresCounter = 0;

		for (auto &o : simulator.bodies)
		{
			if (o.type == TYPE_CIRCLE)
			{
				o.position = spheresPositions[spheresCounter * 2];
				o.shape = spheresPositions[spheresCounter * 2 + 1];
				spheresCounter++;
			}
			else
			if (o.type == TYPE_BOX)
			{
				o.position = cubesPositions[cubesCounter * 2];
				o.shape = cubesPositions[cubesCounter * 2 + 1];
				cubesCounter++;
			}
			else
			if (o.type == TYPE_CILINDRU)
			{
				o.position = cylindrePositions[cylindresCounter * 2];
				o.shape = cylindrePositions[cylindresCounter * 2 + 1];
				cylindresCounter++;
			}
		}
	}


};

bool initGame()
{

	glGenBuffers(2, ssbo);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo[0]);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo[1]);


	glGenBuffers(1, &spheresSSBO);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, spheresSSBO);
	glGenBuffers(1, &cubesSSBO);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, cubesSSBO);
	glGenBuffers(1, &cylindresSSBO);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, cylindresSSBO);

	glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);


	// Generate atomic counter buffers
	glGenBuffers(3, counters);

	std::cout << "spheresSSBO " << spheresSSBO << "\n";
	std::cout << "cubesSSBO " << cubesSSBO << "\n";
	std::cout << "cylindresSSBO " << cylindresSSBO << "\n";


	gpuProfiler.initGPUProfiler();
	//float data[] =
	//{
	//	0, 1, 0,
	//	-1, -1, 0,
	//	1, -1, 0
	//};
	//
	//unsigned short indexData[] = {0,1,2};
	//
	//model.loadFromData(data, sizeof(data), indexData, sizeof(indexData));


	compute.loadComputeShaderProgramFromFile(RESOURCES_PATH "compute.glsl");
	computeu_Size = compute.getUniform("u_size");
	computeu_deltaTime = compute.getUniform("u_deltaTime");
	
	shader.loadShaderProgramFromFile(
		RESOURCES_PATH "shader.vert",
		RESOURCES_PATH "shader.frag"
	);

	viewProj = shader.getUniform("viewProj");
	color = shader.getUniform("color");
	positions = shader.getUniform("positions");
	size = shader.getUniform("size");

	

	instancedShader.loadShaderProgramFromFile(
		RESOURCES_PATH "instancedShader.vert",
		RESOURCES_PATH "instancedShader.frag"
	);
	instancedViewProj = instancedShader.getUniform("viewProj");
	instancedColor = instancedShader.getUniform("color");



	camera.position.z = 2;


	cubeModel.loadFromFile(RESOURCES_PATH "models/cube.obj");
	ballModel.loadFromFile(RESOURCES_PATH "models/ball.obj");
	cilindruModel.loadFromFile(RESOURCES_PATH "models/cilindru.obj");

	cubeVAO = setupMeshVao(cubeModel.meshes[0].mesh, cubesSSBO);
	sphereVAO = setupMeshVao(ballModel.meshes[0].mesh, spheresSSBO);
	cylinderVAO = setupMeshVao(cilindruModel.meshes[0].mesh, cylindresSSBO);


	//for (int i = 0; i < 500; i++)
	//{
	//
	//	simulator.bodies.push_back(createBall(
	//		glm::vec3{getRandomNumber(-1, 1),getRandomNumber(-10, 10),getRandomNumber(-1, 1)}, 
	//		getRandomNumber(0.2, 0.4) * 2));
	//
	//
	//	simulator.bodies.push_back(createBox(
	//		glm::vec3{getRandomNumber(-1, 1),getRandomNumber(-10, 10),getRandomNumber(-1, 1)},
	//		glm::vec3{getRandomNumber(0.2, 1), getRandomNumber(0.2, 1), getRandomNumber(0.2, 1)} * 2.f
	//		));
	//
	//}

	//simulator.bodies.push_back(createCilindru(
	//	glm::vec3{getRandomNumber(-1, 1),getRandomNumber(-10, 10),getRandomNumber(-1, 1)},
	//	getRandomNumber(0.2, 0.4) * 2, getRandomNumber(0.2, 0.4) * 2));

	simulator.bodies.push_back(createCilindru(glm::vec3{0,-10,0}, 2, 1));
	simulator.bodies.push_back(createBall(glm::vec3{0,-5,0}, 1));
	simulator.bodies.push_back(createCilindru(glm::vec3{0,5,0}, 2, 1));
	simulator.bodies.push_back(createCilindru(glm::vec3{0,0,0}, 2, 1));

	//simulator.bodies.push_back(createBox(glm::vec3{0,-2,0}, {2,2,2}));
	//simulator.bodies.push_back(createBox(glm::vec3{0,-7,0}, {2,2,2}));


	//simulator.bodies.push_back(createCilindru(glm::vec3{0,-2,0}, 0.5, 1));
	//simulator.bodies.push_back(createCilindru(glm::vec3{0,7,0}, 0.5, 1));

	//simulator.bodies.push_back(createBall(glm::vec3{}, 0.5));
	//simulator.bodies.push_back(createBall(glm::vec3{1,5,2}, 0.5));
	//simulator.bodies.push_back(createBall(glm::vec3{-4, 6, 7}, 0.5));
	//simulator.bodies.push_back(createBall(glm::vec3{-3, 2, -3}, 0.5));

	uploadDataToGPU();
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

	return true;
}


bool onGPU = 1;
bool instancedVersion = 1;

bool gameLogic(float deltaTime)
{


#pragma region init stuff
	int w = 0; int h = 0;
	w = platform::getFrameBufferSizeX(); //window w
	h = platform::getFrameBufferSizeY(); //window h

	glEnable(GL_DEPTH_TEST);
	glDisable(GL_BLEND);


	glViewport(0, 0, w, h);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); //clear screen


	camera.aspectRatio = (float)w / h;
	if (w == 0 || h == 0) { camera.aspectRatio = 1; }

#pragma endregion

	//compute stuff
	gpuProfiler.startFrame();
	gpuProfiler.startSubProfile("compute work");
	if(onGPU)
	{
		bindDataToGPU();

		compute.bind();
		glUniform1i(computeu_Size, simulator.bodies.size());
		glUniform1f(computeu_deltaTime, deltaTime);

		glDispatchCompute((simulator.bodies.size()+63) / 64, 1, 1);

	}
	gpuProfiler.endSubProfile("compute work");
	gpuProfiler.endFrame();


#pragma region input

	static auto lastMousePos = platform::getRelMousePosition();
	auto newMousePos = platform::getRelMousePosition();
	glm::vec2 mouseDelta = (lastMousePos - newMousePos);
	mouseDelta *= 0.01;
	lastMousePos = newMousePos;

	if (platform::isRMouseHeld())
	{


		camera.rotateCamera(mouseDelta);

		glm::vec3 deplasare = {};
		float speed = deltaTime * 5;

		if (platform::isButtonHeld(platform::Button::A))
		{
			deplasare.x -= speed;
		}

		if (platform::isButtonHeld(platform::Button::D))
		{
			deplasare.x += speed;
		}

		if (platform::isButtonHeld(platform::Button::W))
		{
			deplasare.z -= speed;
		}

		if (platform::isButtonHeld(platform::Button::S))
		{
			deplasare.z += speed;
		}

		if (platform::isButtonHeld(platform::Button::Q))
		{
			deplasare.y -= speed;
		}

		if (platform::isButtonHeld(platform::Button::E))
		{
			deplasare.y += speed;
		}

		camera.moveFps(deplasare);
	}


#pragma endregion

	//fileBrowser.Display();
	//
	//if (fileBrowser.HasSelected())
	//{
	//	std::cout << fileBrowser.GetSelected();
	//	fileBrowser.ClearSelected();
	//}





	ImGui::Begin("Editor");

	cpuProfiler.displayPlot("CPU simulation");

	gpuProfiler.displayPlot("GPU simulation");

	int objectsCount = simulator.bodies.size();
	ImGui::Text("Objects: %d", objectsCount);

	if (ImGui::Button("Clear"))
	{
		simulator.bodies.clear();
		uploadDataToGPU();
	}

	int count = 0;
	if (ImGui::Button("Putine"))
	{
		for (int i = 0; i < 5; i++)
		{

			simulator.bodies.push_back(createBall(
				glm::vec3{getRandomNumber(-1, 1),getRandomNumber(-10, 10),getRandomNumber(-1, 1)},
				getRandomNumber(0.2, 0.4) * 2));
		}


		for (int i = 0; i < 5; i++)
		{
			simulator.bodies.push_back(createBox(
				glm::vec3{getRandomNumber(-1, 1),getRandomNumber(-10, 10),getRandomNumber(-1, 1)},
				glm::vec3{getRandomNumber(0.2, 1), getRandomNumber(0.2, 1), getRandomNumber(0.2, 1)} *2.f
			));

		}

		for (int i = 0; i < 5; i++)
		{
			simulator.bodies.push_back(createCilindru(
				glm::vec3{getRandomNumber(-1, 1),getRandomNumber(-10, 10),getRandomNumber(-1, 1)},
				getRandomNumber(0.2, 0.4) * 2, getRandomNumber(0.2, 0.4) * 2));
		}
		
		uploadDataToGPU();

	}
	if (ImGui::Button("Cub"))
	{
		simulator.bodies.push_back(createBox(
			glm::vec3{getRandomNumber(-1, 1),getRandomNumber(-10, 10),getRandomNumber(-1, 1)},
			glm::vec3{getRandomNumber(0.2, 1), getRandomNumber(0.2, 1), getRandomNumber(0.2, 1)} *2.f
		));
		uploadDataToGPU();

	}
	if (ImGui::Button("Sfera"))
	{
		simulator.bodies.push_back(createBall(
			glm::vec3{getRandomNumber(-1, 1),getRandomNumber(-10, 10),getRandomNumber(-1, 1)},
			getRandomNumber(0.2, 0.4) * 2));
		uploadDataToGPU();

	}
	if (ImGui::Button("Cilindru"))
	{
		simulator.bodies.push_back(createCilindru(
			glm::vec3{getRandomNumber(-1, 1),getRandomNumber(-10, 10),getRandomNumber(-1, 1)},
			getRandomNumber(0.2, 0.4) * 2, getRandomNumber(0.2, 0.4) * 2));
		uploadDataToGPU();

	}
	if (ImGui::Button("100 sfere 250 cub 500 cilindru"))
	{
		count = 1;
	}
	if (ImGui::Button("200 sfere 500 cub 1000 cilindru"))
	{
		count = 2;
	}
	if (ImGui::Button("400 sfere 1000 cub 2000 cilindru"))
	{
		count = 4;
	}

	if (ImGui::Button("Impuls"))
	{
		for (auto &b : simulator.bodies)
		{
			b.acceleration += glm::vec3{getRandomNumber(-2, 2), getRandomNumber(-2, 2), getRandomNumber(-2, 2) * 2000};
		}
		uploadDataToGPU();
	}

	static bool speedup = 0;
	ImGui::Checkbox("Speedup", &speedup);

	ImGui::Checkbox("On GPU", &onGPU);
	ImGui::Checkbox("Instanced Version", &instancedVersion);

	for (int j = 0; j < count; j++)
	{
		for (int i = 0; i < 100; i++)
		{

			simulator.bodies.push_back(createBall(
				glm::vec3{getRandomNumber(-1, 1),getRandomNumber(-10, 10),getRandomNumber(-1, 1)},
				getRandomNumber(0.2, 0.4) * 2));
		}

		for (int i = 0; i < 250; i++)
		{
			simulator.bodies.push_back(createBox(
				glm::vec3{getRandomNumber(-1, 1),getRandomNumber(-10, 10),getRandomNumber(-1, 1)},
				glm::vec3{getRandomNumber(0.2, 1), getRandomNumber(0.2, 1), getRandomNumber(0.2, 1)} *2.f
			));

		}

		for (int i = 0; i < 500; i++)
		{
			simulator.bodies.push_back(createCilindru(
				glm::vec3{getRandomNumber(-1, 1),getRandomNumber(-10, 10),getRandomNumber(-1, 1)},
				getRandomNumber(0.2, 0.4) * 2, getRandomNumber(0.2, 0.4) * 2));
		}
	}
	if (count)
	{
		uploadDataToGPU();
	}


	ImGui::End();
	glEnable(GL_CULL_FACE);

	if(!instancedVersion || !onGPU)
	{
		//normal rendering

		shader.bind();
		glUniformMatrix4fv(viewProj, 1,
			0, glm::value_ptr(camera.getViewProjectionMatrix()));



		ImVec4 colorValue = {0,1,1,1};
		glUniform4f(color, colorValue.x, colorValue.y, colorValue.z, 1);

		for (auto &m : ballModel.meshes)
		{
			glBindVertexArray(m.mesh.vao);

			for (auto &b : simulator.bodies)
			{
				if (b.type == TYPE_CIRCLE)
				{
					glUniform3f(positions, b.position.x, b.position.y, b.position.z);
					glUniform3f(size, b.shape.x * 2, b.shape.x * 2, b.shape.x * 2);
					glDrawElements(GL_TRIANGLES, m.mesh.vertexCount, GL_UNSIGNED_SHORT, 0);
				}
			}
		}

		glUniform4f(color, 1, 1, 0.5, 1);
		for (auto &m : cilindruModel.meshes)
		{
			glBindVertexArray(m.mesh.vao);

			for (auto &b : simulator.bodies)
			{
				if (b.type == TYPE_CILINDRU)
				{
					glUniform3f(positions, b.position.x, b.position.y, b.position.z);
					glUniform3f(size, b.shape.x * 2, b.shape.y, b.shape.x * 2);
					glDrawElements(GL_TRIANGLES, m.mesh.vertexCount, GL_UNSIGNED_SHORT, 0);
				}
			}
		}

		glUniform4f(color, 1, 0.5, 1, 1);
		for (auto &m : cubeModel.meshes)
		{
			glBindVertexArray(m.mesh.vao);

			for (auto &b : simulator.bodies)
			{
				if (b.type == TYPE_BOX)
				{
					glUniform3f(positions, b.position.x, b.position.y, b.position.z);
					glUniform3f(size, b.shape.x, b.shape.y, b.shape.z);
					glDrawElements(GL_TRIANGLES, m.mesh.vertexCount, GL_UNSIGNED_SHORT, 0);
				}
			}
		}


	
	}
	else
	{

		instancedShader.bind();
		glUniformMatrix4fv(instancedViewProj, 1,
			0, glm::value_ptr(camera.getViewProjectionMatrix()));


		ImVec4 colorValue = {0,1,1,1};
		glUniform4f(instancedColor, colorValue.x, colorValue.y, colorValue.z, 1);
		glBindVertexArray(sphereVAO);
		glDrawElementsInstanced(GL_TRIANGLES, ballModel.meshes[0].mesh.vertexCount, GL_UNSIGNED_SHORT, 0, spheresSize);


		glUniform4f(instancedColor, 1, 1, 0.5, 1);
		glBindVertexArray(cylinderVAO);
		glDrawElementsInstanced(GL_TRIANGLES, cilindruModel.meshes[0].mesh.vertexCount, GL_UNSIGNED_SHORT, 0, cylindresSize);

		glUniform4f(instancedColor, 1, 0.5, 1, 1);
		glBindVertexArray(cubeVAO);
		glDrawElementsInstanced(GL_TRIANGLES, cubeModel.meshes[0].mesh.vertexCount, GL_UNSIGNED_SHORT, 0, cubesSize);
		
		glBindVertexArray(0);
	}
	
	shader.bind();
	glUniformMatrix4fv(viewProj, 1,
		0, glm::value_ptr(camera.getViewProjectionMatrix()));

	//draw floor
	for (auto &m : cubeModel.meshes)
	{
		glBindVertexArray(m.mesh.vao);
		glUniform3f(positions, 0, -simulator.boxDimensions.y / 2.f - 0.2, 0);
		glUniform3f(size, simulator.boxDimensions.x, 0.2, simulator.boxDimensions.z);

		glUniform4f(color, 0.5, 0.5, 0.5, 1);

		glDrawElements(GL_TRIANGLES, m.mesh.vertexCount, GL_UNSIGNED_SHORT, 0);
	}


	glEnable(GL_BLEND);
	//glDisable(GL_CULL_FACE);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	for (auto &m : cubeModel.meshes)
	{
		glBindVertexArray(m.mesh.vao);
		glUniform3f(positions, 0, 0, 0);
		glUniform3f(size, simulator.boxDimensions.x, simulator.boxDimensions.y, simulator.boxDimensions.z);

		glUniform4f(color, 0.7, 0.5, 0.5, 0.4);

		glDrawElements(GL_TRIANGLES, m.mesh.vertexCount, GL_UNSIGNED_SHORT, 0);
	}

	glDisable(GL_BLEND);

		

	if(!onGPU)
	{
		cpuProfiler.startSubProfile("Only Simulation: ");

		simulator.update(deltaTime, cpuProfiler);

		if (speedup)
		{
			for (int i = 0; i < 10; i++)
			{
				simulator.update(0.1, cpuProfiler);
			}
		}

		cpuProfiler.endSubProfile("Only Simulation: ");

	}


	//compute stuff
	if(onGPU)
	{
		if (!instancedVersion)
		{
			readDataFromGPU();
		}
		currentShaderReadBuffer = !currentShaderReadBuffer;
	}

	cpuProfiler.endFrame();

	cpuProfiler.startFrame();


	return true;
#pragma endregion

}



void closeGame()
{


}

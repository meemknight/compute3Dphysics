#pragma once
#include <glm/glm.hpp>
#include <vector>
#include <profiler.h>

constexpr int TYPE_CIRCLE = 0;
constexpr int TYPE_BOX = 1;
constexpr int TYPE_CILINDRU = 2;

constexpr static float MAX_VELOCITY = 10'000;
constexpr static float MAX_ACCELERATION = 10'000;
constexpr static float MAX_AIR_DRAG = 100;


struct alignas(16) PhysicsObject
{
	//the position represents the center of the object.
	// the shape is the width height depth for cube and for circle it is radius.

	glm::vec3 position = {};    // 12 bytes
	float padding1;        // 4 bytes

	glm::vec3 shape = {};
	float padding2;

	glm::vec3 velocity = {};
	float padding3;

	glm::vec3 acceleration = {};
	float padding4;

	float mass = 1;
	float bouncyness = 0.9;
	int type =0;
	float staticFriction = 0.4;

	float dynamicFriction = 0.3;
	float padding5[3];     // Align to 16 bytes

	glm::vec3 getMin()
		{
			if (type == TYPE_CIRCLE)
			{
				glm::vec3 rez = position;
				rez -= glm::vec3(shape.x, shape.x, shape.x);
				return rez;
			}
			else if (type == TYPE_BOX)
			{
				glm::vec3 rez = position;
				rez -= glm::vec3(shape) / 2.f;
				return rez;
			}
			else if (type == TYPE_CILINDRU)
			{
				glm::vec3 rez = position;
				rez -= glm::vec3(shape.x, shape.y/2.f, shape.x);
				return rez;
			}
		}

	glm::vec3 getMax()
		{
			if (type == TYPE_CIRCLE)
			{
				glm::vec3 rez = position;
				rez += glm::vec3(shape.x, shape.x, shape.x);
				return rez;
			}
			else if (type == TYPE_BOX)
			{
				glm::vec3 rez = position;
				rez += glm::vec3(shape) / 2.f;
				return rez;
			}if (type == TYPE_CILINDRU)
			{
				glm::vec3 rez = position;
				rez += glm::vec3(shape.x, shape.y / 2.f, shape.x);
				return rez;
			}
		}

};



PhysicsObject createBall(glm::vec3 pos, float r);

PhysicsObject createBox(glm::vec3 pos, glm::vec3 size);

PhysicsObject createCilindru(glm::vec3 pos, float r, float h);


struct Simulator
{
	std::vector<PhysicsObject> bodies;

	void updateForces(PhysicsObject &object, float deltaTime);


	glm::vec3 boxDimensions = {20, 20, 20};

	void update(float deltaTime, Profiler &profiler);

};


#pragma once
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>


class Camera
{
public:

	float aspectRatio = 1;
	float fovRadians = glm::radians(70.f);

	float closePlane = 0.01f;
	float farPlane = 500.f;

	glm::vec3 position = {};

	//unde se uita playerul
	glm::vec3 viewDirection = {0,0,-1};

	glm::mat4x4 getViewMatrix();

	glm::mat4x4 getProjectionMatrix();

	glm::mat4x4 getViewProjectionMatrix();

	void moveFps(glm::vec3 moveDirection);

	void Camera::rotateCamera(glm::vec2 delta);

};
#include "camera.h"


glm::mat4x4 Camera::getViewMatrix()
{
	return  glm::lookAt(position, 
		position + viewDirection, {0,1,0});
}

glm::mat4x4 Camera::getProjectionMatrix()
{
	return glm::perspective(fovRadians, 
		aspectRatio, closePlane, farPlane);
}

glm::mat4x4 Camera::getViewProjectionMatrix()
{
	return getProjectionMatrix() * getViewMatrix();
}


void Camera::moveFps(glm::vec3 moveDirection)
{

	//deplasare fata spate
	position -= moveDirection.z * viewDirection;

	//deplasare sus jos
	position.y += moveDirection.y;

	glm::vec3 left = glm::normalize(glm::cross({0,1,0}, viewDirection));

	//stanga dreapta
	position -= moveDirection.x * left;

}

void Camera::rotateCamera(glm::vec2 delta)
{

	constexpr float PI = 3.14159;

	viewDirection = 
		glm::rotate(glm::mat4(1.f), delta.x, glm::vec3{0,1,0})
		* glm::vec4(viewDirection, 1);

	glm::vec3 left 
		= glm::normalize(glm::cross({0,1,0}, viewDirection));


	float angleUp = glm::acos(glm::dot(glm::vec3{0,1,0}, viewDirection));
	if (angleUp - delta.y < 0.002)
	{
		delta.y = angleUp - 0.002f;
	}

	float angleDown = glm::acos(glm::dot(glm::vec3{0,-1,0}, viewDirection));
	if (angleDown + delta.y < 0.002)
	{
		delta.y = -(angleDown - 0.002f);
	}


	viewDirection =
		glm::rotate(glm::mat4(1.f), -delta.y, left)
		* glm::vec4(viewDirection, 1);




	viewDirection = glm::normalize(viewDirection);

}

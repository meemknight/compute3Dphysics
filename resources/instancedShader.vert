#version 330 core

layout(location = 0) in vec3 in_position;
layout(location = 1) in vec3 in_normal;
layout(location = 2) in vec3 positions;
layout(location = 3) in vec3 size;

uniform mat4 viewProj;


out vec3 normal;

void main()
{

	normal = in_normal;

	gl_Position = viewProj * vec4(in_position * size + positions, 1);
	//gl_Position = viewProj * vec4(in_position * vec3(size.x, size.y, 2) + positions, 1);

}
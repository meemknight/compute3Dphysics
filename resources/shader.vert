#version 330 core

layout(location = 0) in vec3 in_position;
layout(location = 1) in vec3 in_normal;

uniform mat4 viewProj;
uniform vec3 positions;
uniform vec3 size;

out vec3 normal;

void main()
{


	normal = in_normal;

	gl_Position = viewProj * vec4(in_position * size + positions, 1);

}
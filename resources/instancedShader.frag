#version 330 core

layout(location = 0) out vec4 out_color;


in vec3 normal;

uniform vec4 color;


void main()
{

	vec3 sunLight = normalize(vec3(0.2,1,-0.1));
	float lightColor = clamp(dot(normal, sunLight), 0, 1);
	float ambient = 0.4;

	lightColor += ambient;

	lightColor += clamp(dot(normalize(vec3(-0.5, -0.2, 0)), sunLight), 0, 1) * 0.3;

	out_color.rgb = vec3(color) * lightColor;
	out_color.a = color.a;

}


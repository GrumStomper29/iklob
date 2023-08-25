#version 460 core

out vec4 fragColor;

layout (location = 0) in vec3 inNorm;
layout (location = 1) in vec2 inTex;

uniform sampler2D texture0;

void main()
{
	const vec3 lightDir = normalize(const vec3(-2.0f, 8.0f, -1.0f));
	const vec3 lightCol = const vec3(1.0f, 1.0f, 0.71f);

	const float ambientStrength = 0.4f;
	const vec3 ambient = ambientStrength * lightCol;

	float diffuse = max(dot(normalize(inNorm), lightDir), 0.0f);

	fragColor = vec4((diffuse * lightCol + ambient), 1.0f) * vec4(texture(texture0, inTex));
}
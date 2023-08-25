#version 460 core

layout (location = 0) in vec3 inPos;
layout (location = 1) in vec3 inNorm;
layout (location = 2) in vec2 inTex;

layout (location = 0) out vec3 outNorm;
layout (location = 1) out vec2 outTex;

uniform mat4 projection;
uniform mat4 view;
uniform mat4 model;

void main()
{
	gl_Position = projection * view * model * vec4(inPos, 1.0f);

	mat3 normalTransform = inverse(transpose(mat3(model)));
	outNorm = normalTransform * inNorm;

	outTex = inTex;
}
#version 460 core

layout (location = 0) in vec3 inPos;
layout (location = 1) in vec3 inNorm;
layout (location = 2) in vec2 inTex;
layout (location = 3) in ivec4 inJoints;
layout (location = 4) in vec4 inWeights;

layout (location = 0) out vec3 outNorm;
layout (location = 1) out vec2 outTex;

uniform mat4 projection;
uniform mat4 view;
uniform mat4 model;

uniform mat4 jointMatrix[32];

mat4 calculateSkinMatrix()
{
	mat4 skinMatrix = inWeights.x * jointMatrix[inJoints.x] +
		inWeights.y * jointMatrix[inJoints.y] +
		inWeights.z * jointMatrix[inJoints.z] +
		inWeights.w * jointMatrix[inJoints.w];

	return skinMatrix;
}

void main()
{
	
	if (inJoints.x == -1)
	{
	gl_Position = projection * view * model * vec4(inPos, 1.0f);
	}
	else
	{
	gl_Position = projection * view * model * calculateSkinMatrix() * vec4(inPos, 1.0f);
	}

	mat3 normalTransform = inverse(transpose(mat3(model)));
	outNorm = normalTransform * inNorm;
	outTex = inTex;
}
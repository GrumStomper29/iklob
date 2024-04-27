#include "renderer.hpp"

#include "gl_utils.hpp"
#include "model_loader.hpp"

#include "glad/glad.h"
#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtx/quaternion.hpp"
#define SDL_MAIN_HANDLED
#include "SDL.h"

#include <exception>
#include <string> // Todo: consider using string_view for renderMesh() parameter
#include <vector>



void Renderer::init()
{
	if (!gladLoadGLLoader(SDL_GL_GetProcAddress))
	{
		throw std::exception{ "failure loading OpenGL functions" };
	}

	glEnable(GL_DEBUG_OUTPUT);
	glDebugMessageCallback(debugMessageCallback, nullptr);
	glDebugMessageControl(GL_DONT_CARE, GL_DEBUG_TYPE_OTHER, GL_DONT_CARE, 0, nullptr, GL_FALSE);

	m_uberPipeline = Pipeline{ "src/shaders/uber.vert", "src/shaders/uber.frag" };

}

void Renderer::cleanup()
{
	m_uberPipeline = Pipeline{};

	for (const auto& mesh : meshes)
	{
		for (const auto& primitive : mesh.second.primitives)
		{
			if (primitive.material.hasBaseColorTexture)
			{
				glDeleteTextures(1, &primitive.material.baseColorTexture);
			}
		}
	}

	glDeleteBuffers(1, &m_elementBuffer);
	glDeleteVertexArrays(1, &m_vertexArray);
	glDeleteBuffers(1, &m_vertexBuffer);
}



void Renderer::beginRendering(const glm::vec3& cameraPosition, const glm::vec3& cameraLook,
	float fieldOfView, float aspectRatio, float nearPlane, float farPlane, const glm::vec3& lightColor)
{
	glm::mat4 view{ glm::lookAt(cameraPosition, cameraPosition + cameraLook, glm::vec3{ 0.0f, 1.0f, 0.0f }) };
	glm::mat4 projection{ glm::perspective(glm::radians(fieldOfView), aspectRatio, nearPlane, farPlane) };

	glViewport(0, 0, m_viewportWidth, m_viewportHeight);

	glEnable(GL_DEPTH_TEST);

	glClearColor(0.9f, 0.9f, 1.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	m_uberPipeline.bind();
	m_uberPipeline.setUniformMat4("projection", projection);
	m_uberPipeline.setUniformMat4("view", view);

	m_uberPipeline.setUniformVec3("lightColor", lightColor);

	glBindVertexArray(m_vertexArray);
}

void Renderer::renderMesh(const std::string& mesh, const glm::mat4& transform)
{
	if (meshes.at(mesh).joints.size() != 0)
	{
		auto jointMatrix{ calculateJointMatrix(meshes.at(mesh)) };
		jointMatrix.resize(32);
		m_uberPipeline.setUniformMat4Array("jointMatrix", jointMatrix);
	}

	for (const Primitive& primitive : meshes.at(mesh).primitives)
	{
		m_uberPipeline.setUniformMat4("model", transform * primitive.transform);

		glBindTextureUnit(0, primitive.material.baseColorTexture);
		m_uberPipeline.setUniformInt("texture0", 0);

		glDrawElements(GL_TRIANGLES, primitive.elementCount, GL_UNSIGNED_INT,
			reinterpret_cast<const void*>(sizeof(GLuint) * primitive.elementOffset));
	}
}



void Renderer::setViewport(SDL_Window* window)
{
	SDL_GL_GetDrawableSize(window, &m_viewportWidth, &m_viewportHeight);
}



void Renderer::loadScene(int modelPathCount, std::pair<std::string, std::string>* modelPaths)
{
	for (int i{ 0 }; i < modelPathCount; ++i)
	{
		meshes[modelPaths[i].second] = loadModel(modelPaths[i].first, m_sceneVertices, m_sceneIndices);
	}

	glCreateBuffers(1, &m_vertexBuffer);
	glNamedBufferStorage(m_vertexBuffer, sizeof(Vertex) * m_sceneVertices.size(), m_sceneVertices.data(), 0);

	glCreateBuffers(1, &m_elementBuffer);
	glNamedBufferStorage(m_elementBuffer, sizeof(GLuint) * m_sceneIndices.size(), m_sceneIndices.data(), 0);
	m_elementBufferElementCount = static_cast<GLsizei>(m_sceneIndices.size());

	glCreateVertexArrays(1, &m_vertexArray);

	glVertexArrayVertexBuffer(m_vertexArray, 0, m_vertexBuffer, 0, sizeof(Vertex));
	glVertexArrayElementBuffer(m_vertexArray, m_elementBuffer);

	glEnableVertexArrayAttrib(m_vertexArray, 0);
	glEnableVertexArrayAttrib(m_vertexArray, 1);
	glEnableVertexArrayAttrib(m_vertexArray, 2);
	glEnableVertexArrayAttrib(m_vertexArray, 3);
	glEnableVertexArrayAttrib(m_vertexArray, 4);

	glVertexArrayAttribFormat(m_vertexArray, 0, 3, GL_FLOAT, GL_FALSE, offsetof(Vertex, position));
	glVertexArrayAttribFormat(m_vertexArray, 1, 3, GL_FLOAT, GL_FALSE, offsetof(Vertex, normal));
	glVertexArrayAttribFormat(m_vertexArray, 2, 2, GL_FLOAT, GL_FALSE, offsetof(Vertex, texCoord));
	glVertexArrayAttribIFormat(m_vertexArray, 3, 4, GL_INT,   offsetof(Vertex, joints));
	glVertexArrayAttribFormat(m_vertexArray, 4, 4, GL_FLOAT, GL_FALSE, offsetof(Vertex, weights));

	glVertexArrayAttribBinding(m_vertexArray, 0, 0);
	glVertexArrayAttribBinding(m_vertexArray, 1, 0);
	glVertexArrayAttribBinding(m_vertexArray, 2, 0);
	glVertexArrayAttribBinding(m_vertexArray, 3, 0);
	glVertexArrayAttribBinding(m_vertexArray, 4, 0);
}



glm::vec4 Renderer::determineSamplerOutput(const AnimationSampler& sampler, double time, double maxTime)
{
	float lesserInputValue{ -1.0f };
	int lesserInput{};

	float greaterInputValue{ static_cast<float>(maxTime) + 1.0f };
	int greaterInput{};

	for (int i{ 0 }; i < sampler.input.size(); ++i)
	{
		if (sampler.input[i] <= time && sampler.input[i] > lesserInputValue)
		{
			lesserInputValue = sampler.input[i];
			lesserInput = i;
		}

		if (sampler.input[i] >= time && sampler.input[i] < greaterInputValue)
		{
			greaterInputValue = sampler.input[i];
			greaterInput = i;
		}
	}

	if (time == sampler.input[lesserInput])
	{
		return sampler.output[lesserInput];
	}
	else
	{
		const float x{ (static_cast<float>(time) - lesserInputValue) / (greaterInputValue - lesserInputValue) };

		return glm::mix(sampler.output[lesserInput], sampler.output[greaterInput], x);
	}
}

glm::mat4 Renderer::calculateJointLocalTransform(const Joint& joint, double time, double maxTime)
{
	glm::mat4 transform{ 1.0f };

	for (const auto& sampler : joint.animationSamplers)
	{
		glm::vec4 output{ determineSamplerOutput(sampler.second, time, maxTime) };

		switch (sampler.second.path)
		{
		case AnimationSampler::TRANSLATION:
			glm::mat4 translation{ 1.0f };
			translation = glm::translate(translation, static_cast<glm::vec3>(output));
			transform *= translation;
			break;

		case AnimationSampler::ROTATION:
			glm::quat quat{ output.w, output.x, output.y, output.z };
			glm::mat4 rotation{ glm::toMat4(quat) };
			transform *= rotation;
			break;

		case AnimationSampler::SCALE:
			glm::mat4 scale{ 1.0f };
			scale = glm::scale(scale, static_cast<glm::vec3>(output));
			transform *= scale;
			break;
		}
	}

	return transform;
}

void Renderer::calculateJointChildrenGlobalTransforms(const Mesh& mesh,
	const Joint& joint, const glm::mat4& jointGlobalTransform,
	const std::vector<glm::mat4>& localTransforms, std::vector<glm::mat4>& globalTransforms)
{
	for (int childIndex : joint.children)
	{
		glm::mat4 childGlobalTransform{ jointGlobalTransform * localTransforms[childIndex] };

		globalTransforms[childIndex] = childGlobalTransform;

		if (mesh.joints[childIndex].children.size() != 0)
		{
			calculateJointChildrenGlobalTransforms(mesh, mesh.joints[childIndex], childGlobalTransform,
				localTransforms, globalTransforms);
		}
	}
}

std::vector<glm::mat4> Renderer::calculateJointMatrix(const Mesh& mesh)
{
	std::vector<glm::mat4> jointMatrix(mesh.joints.size());

	// Transform of the node that the mesh is attached to
	glm::mat4 globalTransform{ 1.0f };
	
	std::vector<glm::mat4> localTransforms(mesh.joints.size());
	for (int i{ 0 }; i < mesh.joints.size(); ++i)
	{
		localTransforms[i] = calculateJointLocalTransform(mesh.joints[i], mesh.time, mesh.maxTime);
	}

	// Global transform of the joint. This needs to inherit all its transforms from
	// parent joints.
	std::vector<glm::mat4> globalJointTransforms(mesh.joints.size());
	for (int i{ 0 }; i < globalJointTransforms.size(); ++i)
	{
		if (!mesh.joints[i].hasJointParent)
		{
			globalJointTransforms[i] = localTransforms[i];

			calculateJointChildrenGlobalTransforms(mesh, mesh.joints[i], localTransforms[i],
				localTransforms, globalJointTransforms);
		}
	}
	
	for (int i{ 0 }; i < mesh.joints.size(); ++i)
	{
		glm::mat4 globalTransform{ 1.0f };

		jointMatrix[i] = { glm::mat4{
			glm::inverse(globalTransform) * 
			globalJointTransforms[i] * 
			mesh.joints[i].inverseBindMatrix
		} };
	}

	return jointMatrix;
}


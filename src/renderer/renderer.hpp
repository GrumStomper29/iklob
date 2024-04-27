#pragma once

#include "pipeline.hpp"

#include "glad/glad.h"
#include "glm/glm.hpp"
#define SDL_MAIN_HANDLED
#include "SDL/SDL.h"

#include <map>
#include <string>
#include <unordered_map>
#include <vector>

class Renderer
{
public:

	struct Vertex
	{
		glm::vec3 position{};
		glm::vec3 normal{};
		glm::vec2 texCoord{};
		glm::ivec4 joints{};
		glm::vec4 weights{};
	};

	struct Material
	{
		glm::vec4 baseColorFactor{ 1.0f, 1.0f, 1.0f, 1.0f };
		bool hasBaseColorTexture{ false };
		GLuint baseColorTexture{};
	};

	struct Primitive
	{
		Material material{};

		glm::mat4 transform{ 1.0f };

		GLsizei elementOffset{};
		GLsizei elementCount{};
	};

	struct AnimationSampler
	{
		enum Path
		{
			TRANSLATION,
			ROTATION,
			SCALE,
		};

		Path path{};
		std::vector<float> input{};
		std::vector<glm::vec4> output{};
	};

	struct Joint
	{
		std::string name{};
		int nodeIndex{};

		bool hasJointParent{ false };
		std::vector<int> children{};

		glm::mat4 transform{};
		glm::mat4 inverseBindMatrix{};

		std::multimap<AnimationSampler::Path, AnimationSampler> animationSamplers{};
	};

	struct Mesh
	{
		std::vector<Primitive> primitives{};
		std::vector<Joint> joints{};

		double time{ 0.0f };
		double maxTime{ 1.0f };
	};

	void init();
	void cleanup();

	void beginRendering(const glm::vec3& cameraPosition, const glm::vec3& cameraLook,
		float fieldOfView, float aspectRatio, float nearPlane, float farPlane, const glm::vec3& lightColor);

	void renderMesh(const std::string& mesh, const glm::mat4& transform);

	void setViewport(SDL_Window* window);

	void loadScene(int modelPathCount, std::pair<std::string, std::string>* modelPaths);

	const std::vector<Vertex>& sceneVertices() const
	{
		return m_sceneVertices;
	}
	const std::vector<GLuint>& sceneIndices() const
	{
		return m_sceneIndices;
	}

	std::unordered_map<std::string, Mesh> meshes{};

private:

	int m_viewportWidth{};
	int m_viewportHeight{};

	GLuint  m_vertexBuffer{};
	GLuint  m_vertexArray{};
	GLuint  m_elementBuffer{};
	GLsizei m_elementBufferElementCount{};

	std::vector<Vertex> m_sceneVertices{};
	std::vector<GLuint> m_sceneIndices{};

	Pipeline m_uberPipeline{};


	glm::vec4 determineSamplerOutput(const AnimationSampler& sampler, double time, double maxTime);

	glm::mat4 calculateJointLocalTransform(const Joint& joint, double time, double maxTime);

	void calculateJointChildrenGlobalTransforms(const Mesh& mesh,
		const Joint& joint, const glm::mat4& jointGlobalTransform,
		const std::vector<glm::mat4>& localTransforms, std::vector<glm::mat4>& globalTransforms);

	std::vector<glm::mat4> calculateJointMatrix(const Mesh& mesh);

};
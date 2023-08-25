#pragma once

#include "pipeline.hpp"

#include "glad/glad.h"
#include "glm/glm.hpp"
#define SDL_MAIN_HANDLED
#include "SDL/SDL.h"

#include <string>
#include <vector>

class Renderer
{
public:

	struct Vertex
	{
		glm::vec3 position{};
		glm::vec3 normal{};
		glm::vec2 texCoord{};
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

	struct Mesh
	{
		std::vector<Primitive> primitives{};
	};

	void init();
	void cleanup();

	void render(const glm::vec3& cameraPosition, const glm::vec3& cameraLook,
		float fieldOfView, float aspectRatio, float nearPlane, float farPlane);

	void setViewport(SDL_Window* window);

	void loadScene(int modelPathCount, const std::string* modelPaths);

private:

	int m_viewportWidth{};
	int m_viewportHeight{};

	std::vector<Mesh> m_meshes{};

	GLuint  m_vertexBuffer{};
	GLuint  m_vertexArray{};
	GLuint  m_elementBuffer{};
	GLsizei m_elementBufferElementCount{};

	Pipeline m_uberPipeline{};

};
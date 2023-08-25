#include "renderer.hpp"

#include "gl_utils.hpp"
#include "model_loader.hpp"

#include "glad/glad.h"
#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "SDL.h"

#include <exception>
#include <string>
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

	for (Mesh& mesh : m_meshes)
	{
		for (Primitive& primitive : mesh.primitives)
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



void Renderer::render(const glm::vec3& cameraPosition, const glm::vec3& cameraLook,
	float fieldOfView, float aspectRatio, float nearPlane, float farPlane)
{
	glm::mat4 model{ 1.0f };
	glm::mat4 view{ glm::lookAt(cameraPosition, cameraPosition + cameraLook, glm::vec3{ 0.0f, 1.0f, 0.0f }) };
	glm::mat4 projection{ glm::perspective(glm::radians(fieldOfView), aspectRatio, nearPlane, farPlane) };

	glViewport(0, 0, m_viewportWidth, m_viewportHeight);

	glEnable(GL_DEPTH_TEST);

	glClearColor(0.9f, 0.9f, 1.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	m_uberPipeline.bind();
	m_uberPipeline.setUniformMat4("projection", projection);
	m_uberPipeline.setUniformMat4("view", view);

	glBindVertexArray(m_vertexArray);
	
	for (const Mesh& mesh : m_meshes)
	{
		for (const Primitive& primitive : mesh.primitives)
		{
			m_uberPipeline.setUniformMat4("model", model * primitive.transform);

			glBindTextureUnit(0, primitive.material.baseColorTexture);
			m_uberPipeline.setUniformInt("texture0", 0);

			glDrawElements(GL_TRIANGLES, primitive.elementCount, GL_UNSIGNED_INT, 
				reinterpret_cast<const void*>(sizeof(GLuint) * primitive.elementOffset));
		}
	}
}



void Renderer::setViewport(SDL_Window* window)
{
	SDL_GL_GetDrawableSize(window, &m_viewportWidth, &m_viewportHeight);
}



void Renderer::loadScene(int modelPathCount, const std::string* modelPaths)
{
	std::vector<Vertex> vertices{};
	std::vector<GLuint> indices{};

	for (int i{ 0 }; i < modelPathCount; ++i)
	{
		m_meshes.push_back(loadModel(modelPaths[i], vertices, indices));
	}

	glCreateBuffers(1, &m_vertexBuffer);
	glNamedBufferStorage(m_vertexBuffer, sizeof(Vertex) * vertices.size(), vertices.data(), 0);

	glCreateBuffers(1, &m_elementBuffer);
	glNamedBufferStorage(m_elementBuffer, sizeof(GLuint) * indices.size(), indices.data(), 0);
	m_elementBufferElementCount = indices.size();

	glCreateVertexArrays(1, &m_vertexArray);

	glVertexArrayVertexBuffer(m_vertexArray, 0, m_vertexBuffer, 0, sizeof(Vertex));
	glVertexArrayElementBuffer(m_vertexArray, m_elementBuffer);

	glEnableVertexArrayAttrib(m_vertexArray, 0);
	glEnableVertexArrayAttrib(m_vertexArray, 1);
	glEnableVertexArrayAttrib(m_vertexArray, 2);

	glVertexArrayAttribFormat(m_vertexArray, 0, 3, GL_FLOAT, GL_FALSE, offsetof(Vertex, position));
	glVertexArrayAttribFormat(m_vertexArray, 1, 3, GL_FLOAT, GL_FALSE, offsetof(Vertex, normal));
	glVertexArrayAttribFormat(m_vertexArray, 2, 2, GL_FLOAT, GL_FALSE, offsetof(Vertex, texCoord));

	glVertexArrayAttribBinding(m_vertexArray, 0, 0);
	glVertexArrayAttribBinding(m_vertexArray, 1, 0);
	glVertexArrayAttribBinding(m_vertexArray, 2, 0);
}
#pragma once

#include "glad/glad.h"
#include "glm/glm.hpp"

#include <string>
#include <vector>

class Pipeline
{
public:
	Pipeline() = default;
	Pipeline(const std::string& vertexShaderPath, const std::string& fragmentShaderPath);

	Pipeline(const Pipeline&) = delete;
	Pipeline& operator=(const Pipeline&) = delete;

	Pipeline(Pipeline&& p) noexcept;
	Pipeline& operator=(Pipeline&& p) noexcept;

	~Pipeline();

	void bind();

	void setUniformInt(const std::string& name, int value);
	void setUniformMat4(const std::string& name, const glm::mat4& value);
	void setUniformMat4Array(const std::string& name, const std::vector<glm::mat4>& matrices);

private:
	GLuint m_program{};

	bool m_initialized{ false };

	void moveFrom(Pipeline&& p);
	void destruct();
};
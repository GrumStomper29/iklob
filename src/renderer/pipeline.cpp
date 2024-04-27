#include "pipeline.hpp"

#include "gl_utils.hpp"

#include "glm/glm.hpp"
#include "glm/gtc/type_ptr.hpp"

#include <iostream>
#include <string>
#include <utility>
#include <vector>

Pipeline::Pipeline(const std::string& vertexShaderPath, const std::string& fragmentShaderPath)
{
	GLuint vertexShader{ compileShader(vertexShaderPath, GL_VERTEX_SHADER) };
	GLuint fragmentShader{ compileShader(fragmentShaderPath, GL_FRAGMENT_SHADER) };

	m_program = glCreateProgram();
	glAttachShader(m_program, vertexShader);
	glAttachShader(m_program, fragmentShader);
	glLinkProgram(m_program);

	GLint success{};
	glGetProgramiv(m_program, GL_LINK_STATUS, &success);
	if (!success)
	{
		GLchar infoLog[1024]{};
		glGetProgramInfoLog(m_program, 1024, nullptr, infoLog);
		std::cerr << infoLog << '\n';
	}

	glDeleteShader(vertexShader);
	glDeleteShader(fragmentShader);

	m_initialized = true;
}

Pipeline::Pipeline(Pipeline&& p) noexcept
{
	moveFrom(std::move(p));
}

Pipeline& Pipeline::operator=(Pipeline&& p) noexcept
{
	destruct();
	moveFrom(std::move(p));
	return *this;
}

Pipeline::~Pipeline()
{
	destruct();
}



void Pipeline::bind()
{
	glUseProgram(m_program);
}

void Pipeline::setUniformInt(const std::string& name, int value)
{
	GLint location{ glGetUniformLocation(m_program, name.c_str()) };
	glUniform1i(location, value);
}

void Pipeline::setUniformMat4(const std::string& name, const glm::mat4& value)
{
	GLint location{ glGetUniformLocation(m_program, name.c_str()) };
	glUniformMatrix4fv(location, 1, GL_FALSE, glm::value_ptr(value));
}

void Pipeline::setUniformMat4Array(const std::string& name, const std::vector<glm::mat4>& matrices)
{
	GLint location{ glGetUniformLocation(m_program, name.c_str()) };
	glUniformMatrix4fv(location, matrices.size(), GL_FALSE, glm::value_ptr(matrices[0]));
}

void Pipeline::setUniformVec3(const std::string& name, const glm::vec3& value)
{
	GLint location{ glGetUniformLocation(m_program, name.c_str()) };
	glUniform3fv(location, 1, glm::value_ptr(value));
}



void Pipeline::moveFrom(Pipeline&& p)
{
	m_program     = p.m_program;
	m_initialized = p.m_initialized;

	p.m_program     = 0u;
	p.m_initialized = false;
}

void Pipeline::destruct()
{
	if (m_initialized)
	{
		glDeleteProgram(m_program);
		m_initialized = false;
	}
}
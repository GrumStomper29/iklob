#include "gl_utils.hpp"

#include "glad/glad.h"

#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

// Thanks to fendevel for this function
void debugMessageCallback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, GLchar const* message, void const* user_param)
{
	auto const src_str{ [source]() {
		switch (source)
		{
		case GL_DEBUG_SOURCE_API:             return "API";
		case GL_DEBUG_SOURCE_WINDOW_SYSTEM:   return "WINDOW SYSTEM";
		case GL_DEBUG_SOURCE_SHADER_COMPILER: return "SHADER COMPILER";
		case GL_DEBUG_SOURCE_THIRD_PARTY:     return "THIRD PARTY";
		case GL_DEBUG_SOURCE_APPLICATION:     return "APPLICATION";
		case GL_DEBUG_SOURCE_OTHER:           return "OTHER";
		} }() };

	auto const type_str{ [type]() {
		switch (type)
		{
		case GL_DEBUG_TYPE_ERROR:               return "ERROR";
		case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR: return "DEPRECATED_BEHAVIOR";
		case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR:  return "UNDEFINED_BEHAVIOR";
		case GL_DEBUG_TYPE_PORTABILITY:         return "PORTABILITY";
		case GL_DEBUG_TYPE_PERFORMANCE:         return "PERFORMANCE";
		case GL_DEBUG_TYPE_MARKER:              return "MARKER";
		case GL_DEBUG_TYPE_OTHER:               return "OTHER";
		} }() };

	auto const severity_str{ [severity]() {
		switch (severity) {
		case GL_DEBUG_SEVERITY_NOTIFICATION: return "NOTIFICATION";
		case GL_DEBUG_SEVERITY_LOW:          return "LOW";
		case GL_DEBUG_SEVERITY_MEDIUM:       return "MEDIUM";
		case GL_DEBUG_SEVERITY_HIGH:         return "HIGH";
		} }() };

	std::cerr << src_str << ", " << type_str << ", " << severity_str << ", " << id << ": " << message << '\n';
}

GLuint compileShader(const std::string& filename, GLenum type)
{
	std::ifstream inputStream{ filename };

	std::stringstream stringStream{};
	stringStream << inputStream.rdbuf();

	std::string srcStr{ stringStream.str() };
	const char* srcCStr{ srcStr.c_str() };

	GLuint shader{ glCreateShader(type) };
	glShaderSource(shader, 1, &srcCStr, nullptr);
	glCompileShader(shader);

	GLint success{};
	glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
	if (!success)
	{
		GLchar infoLog[1024]{};
		glGetShaderInfoLog(shader, 1024, nullptr, infoLog);
		std::cerr << infoLog << '\n';
	}

	return shader;
}

GLuint createTexture(const void* pixels, GLsizei width, GLsizei height, GLint minFilter, GLint magFilter, int bits)
{
	GLuint texture{};
	glCreateTextures(GL_TEXTURE_2D, 1, &texture);

	glTextureParameteri(texture, GL_TEXTURE_MIN_FILTER, minFilter);
	glTextureParameteri(texture, GL_TEXTURE_MAG_FILTER, magFilter);

	GLenum format { [=]()->GLenum {
			switch (bits)
			{
			case 8:  return GL_RGBA8;
			case 16: return GL_RGBA16;
			default: return GL_RGBA8;
			}
		}() };

	GLenum type{ [=]()->GLenum {
			switch (bits)
			{
			case 8:  return GL_UNSIGNED_BYTE;
			case 16: return GL_UNSIGNED_SHORT;
			default: return GL_UNSIGNED_BYTE;
			}
		}() };

	glTextureStorage2D(texture, 1, format, width, height);
	glTextureSubImage2D(texture, 0, 0, 0, width, height, GL_RGBA, type, pixels);
	glGenerateTextureMipmap(texture);

	return texture;
}
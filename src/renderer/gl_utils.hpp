#pragma once

#include "glad/glad.h"

#include <string>

void debugMessageCallback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, GLchar const* message, void const* user_param);

GLuint compileShader(const std::string& filename, GLenum type);

GLuint createTexture(const void* pixels, GLsizei width, GLsizei height, GLint minFilter, GLint magFilter);
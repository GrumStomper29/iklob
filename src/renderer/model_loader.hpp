#pragma once

#include "renderer.hpp"

#include "glad/glad.h"

#include <string>
#include <vector>

Renderer::Mesh loadModel(const std::string& path, std::vector<Renderer::Vertex>& vertices, std::vector<GLuint>& indices);
#pragma once

#include "../renderer/renderer.hpp"

#include "glm/glm.hpp"

void calculatePrimitiveBounds(const Renderer& renderer, Renderer::Primitive& primitive);

bool intersectCylinderWithMesh(glm::vec3 cylinderOrigin, glm::vec3 cylinderDirection,
	float cylinderRadius, float cylinderHeight, const Renderer& renderer, const Renderer::Mesh& mesh,
	glm::vec3& outNormal);

bool intersectCylinderWithMeshCulled(glm::vec3 cylinderOrigin, glm::vec3 cylinderDirection,
	float cylinderRadius, float cylinderHeight, const Renderer& renderer, const Renderer::Mesh& mesh,
	glm::vec3& outNormal, const glm::vec3& cullDir);
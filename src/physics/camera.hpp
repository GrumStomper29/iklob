#pragma once

#include "../input/input.hpp"
#include "../renderer/renderer.hpp"

#include "glm/glm.hpp"

struct Camera
{
	glm::vec3 position{};

	float yaw{ -90.0f };
	float pitch{};

	glm::vec3 forwardVector{};
	glm::vec3 rightVector{};

	glm::vec3 horizontalForwardVector{};
	glm::vec3 horizontalRightVector{};

	float forwardVelocity{};
	float rightVelocity{};
	float upVelocity{};

	const float moveSpeed{ 5.0f };
	const float lookSpeed{ 10.0f };

	float airTime{ 0.1f };
};

// Also calculates the cameras right vector.
void calculateCameraFrontVector(Camera& camera);

void updateCamera(Camera& camera, const Input& input, const Renderer& renderer, float deltaTime);
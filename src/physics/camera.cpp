#include "camera.hpp"

#include "../input/input.hpp"
#include "collision_detection.hpp"

#include "glm/glm.hpp"
#include "glm/gtx/projection.hpp"

#include <cmath>

void calculateCameraFrontVector(Camera& camera)
{
	camera.forwardVector = {
		std::cos(glm::radians(camera.yaw)) * std::cos(glm::radians(camera.pitch)),
		std::sin(glm::radians(camera.pitch)),
		std::sin(glm::radians(camera.yaw)) * std::cos(glm::radians(camera.pitch)) };

	camera.forwardVector = glm::normalize(camera.forwardVector);

	camera.rightVector = glm::normalize(glm::cross(glm::vec3{ 0.0f, 1.0f, 0.0f }, camera.forwardVector));

	camera.horizontalForwardVector = {
		std::cos(glm::radians(camera.yaw)),
		0.0f,
		std::sin(glm::radians(camera.yaw)) };

	camera.horizontalForwardVector = glm::normalize(camera.horizontalForwardVector);

	camera.horizontalRightVector = glm::normalize(glm::cross(glm::vec3{ 0.0f, 1.0f, 0.0f }, camera.horizontalForwardVector));
}

void updateCamera(Camera& camera, const Input& input, const Renderer& renderer, float deltaTime)
{
	glm::vec3 colNorm{};

	auto testForCollision{ [&]() {
			bool res = intersectCylinderWithMesh(camera.position, glm::vec3{ 0.0f, 1.0f, 0.0f }, 0.5f, 2.0f, renderer, renderer.m_meshes[0], colNorm);
			return res;
		} };

	auto testForCollision2{ [&](const glm::vec3& cullDir) {
			bool res = intersectCylinderWithMeshCulled(camera.position, glm::vec3{ 0.0f, 1.0f, 0.0f }, 0.5f, 2.0f, renderer, renderer.m_meshes[0], colNorm,
				cullDir);
			return res;
		} };

	calculateCameraFrontVector(camera);

	camera.airTime += deltaTime;

	if (input.getKeyDown(Input::W))
	{
		camera.forwardVelocity += -camera.moveSpeed * deltaTime;
	}
	else if (input.getKeyDown(Input::S))
	{
		camera.forwardVelocity += camera.moveSpeed * deltaTime;
	}
	else
	{
		//camera.forwardVelocity = 0.0f;
	}

	if (input.getKeyDown(Input::D))
	{
		camera.rightVelocity += camera.moveSpeed * deltaTime;
	}
	else if (input.getKeyDown(Input::A))
	{
		camera.rightVelocity += -camera.moveSpeed * deltaTime;
	}
	else
	{
		//camera.rightVelocity = 0.0f;
	}

	if (input.getKeyDown(Input::SPACE) && camera.airTime < 0.1f)
	{
		camera.upVelocity = 0.3f;
	}

	camera.upVelocity -= 1.0f * deltaTime;

	camera.rightVelocity *= 0.7f;
	camera.forwardVelocity *= 0.7f;

	camera.position.y += camera.upVelocity;
	if (testForCollision())
	{
		do
		{
			camera.position.y -= camera.upVelocity / 4.0f;
		} while (testForCollision());

		camera.upVelocity = 0.0f;
		camera.airTime = 0.0f;
	}

	camera.position -= camera.horizontalForwardVector * camera.forwardVelocity;

	if (testForCollision2(camera.forwardVelocity > 0 ? -camera.horizontalForwardVector : camera.horizontalForwardVector))
	{
		camera.position += camera.horizontalForwardVector * camera.forwardVelocity;

		glm::vec3 surfacePush{ glm::proj(camera.horizontalForwardVector, colNorm) };
		camera.position -= (camera.horizontalForwardVector - surfacePush) * camera.forwardVelocity;

		if (testForCollision())
		{
			camera.position += (camera.horizontalForwardVector - surfacePush) * camera.forwardVelocity;
		}
	}

	camera.position -= camera.horizontalRightVector * camera.rightVelocity;

	if (testForCollision2(camera.rightVelocity > 0 ? -camera.horizontalRightVector : camera.horizontalRightVector))
	{
		camera.position += camera.horizontalRightVector * camera.rightVelocity;

		glm::vec3 surfacePush{ glm::proj(camera.horizontalRightVector, colNorm) };
		camera.position -= (camera.horizontalRightVector - surfacePush) * camera.rightVelocity;

		if (testForCollision())
		{
			camera.position += (camera.horizontalRightVector - surfacePush) * camera.rightVelocity;
		}
	}
}
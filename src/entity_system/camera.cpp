#include "camera.hpp"

#include "../input/input.hpp"

#include "glm/glm.hpp"
#include "PxPhysicsAPI.h"

#include <cmath>

void Camera::calculateFrontVec()
{
	m_forwardVec = 
	{
		std::cos(glm::radians(m_yaw)) * std::cos(glm::radians(m_pitch)),
		std::sin(glm::radians(m_pitch)),
		std::sin(glm::radians(m_yaw)) * std::cos(glm::radians(m_pitch)) 
	};
	m_forwardVec = glm::normalize(m_forwardVec);

	m_rightVec = glm::normalize(glm::cross({ 0.0f, 1.0f, 0.0f }, m_forwardVec));
}

// Assumes calculateFrontVec() has already been called
void Camera::update(const Input& input, float deltaTime)
{
	glm::vec3 forwardHorizontal
	{
		std::cos(glm::radians(m_yaw)) * std::cos(glm::radians(m_pitch)),
		0.0f,
		std::sin(glm::radians(m_yaw)) * std::cos(glm::radians(m_pitch))
	};
	forwardHorizontal = glm::normalize(forwardHorizontal);

	if (input.getKeyDown(Input::W))
	{
		m_vel += (forwardHorizontal * m_moveSpeed) * deltaTime;
	}
	if (input.getKeyDown(Input::S))
	{
		m_vel += (forwardHorizontal * -m_moveSpeed) * deltaTime;
	}
	if (input.getKeyDown(Input::A))
	{
		m_vel += (m_rightVec * m_moveSpeed) * deltaTime;
	}
	if (input.getKeyDown(Input::D))
	{
		m_vel += (m_rightVec * -m_moveSpeed) * deltaTime;
	}
	if (input.getKeyDown(Input::SPACE) && airTime < 0.3f)
	{
		m_vel.y = 6.0f;
		airTime = 0.3f;
	}

	// Naive friction/air resistance
	m_vel.x *= 0.8f;
	m_vel.z *= 0.8f;
	
	// Gravity
	m_vel.y -= 9.8f * deltaTime;

	physx::PxVec3 disp{ m_vel.x * deltaTime, m_vel.y * deltaTime, m_vel.z * deltaTime };

	physx::PxControllerFilters filters{};
	auto collisionFlags{ m_controller->move(disp, 0.01f, deltaTime, filters) };

	if (collisionFlags & physx::PxControllerCollisionFlag::eCOLLISION_DOWN
		|| collisionFlags & physx::PxControllerCollisionFlag::eCOLLISION_UP)
	{
		m_vel.y = 0.0f;
		airTime = 0.0f;
	}

	airTime += deltaTime;
}

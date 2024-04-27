#pragma once

#include "../input/input.hpp"

#include "glm/glm.hpp"
#include "PxPhysicsAPI.h"

class Camera final
{
public:

	Camera(const physx::PxExtendedVec3& pos, physx::PxControllerManager* controllerManager,
		physx::PxMaterial* material)
	{
		calculateFrontVec();

		physx::PxCapsuleControllerDesc controllerDesc{};
		controllerDesc.radius = 0.5f;
		controllerDesc.height = 1.5;
		controllerDesc.position = pos;
		controllerDesc.material = material;
		m_controller = controllerManager->createController(controllerDesc);
	}
	Camera(const physx::PxExtendedVec3& pos, physx::PxControllerManager* controllerManager,
		physx::PxMaterial* material, float yaw, float pitch)
		: Camera(pos, controllerManager, material)
	{
		m_yaw = yaw;
		m_pitch = pitch;
	}

	~Camera()
	{
		m_controller->release();
	}

	Camera() = delete;
	Camera(Camera&) = delete;
	Camera& operator=(Camera&) = delete;

	float m_yaw{ -90.0f };
	float m_pitch{ 0.0f };

	float m_moveSpeed{ 100.0f };
	float m_lookSpeed{ 10.0f };

	const glm::vec3& getForwardVec() const
	{
		return m_forwardVec;
	}

	physx::PxExtendedVec3 getPos() const
	{
		auto pos{ m_controller->getPosition() };
		pos.y += 1.2f;

		return pos;
	}

	void setVel(const glm::vec3& vel)
	{
		m_vel = vel;
	}
	const glm::vec3 getVel() const
	{
		return m_vel;
	}

	void calculateFrontVec();
	void update(const Input& input, float deltaTime);

private:

	physx::PxController* m_controller{};

	glm::vec3 m_forwardVec{};
	glm::vec3 m_rightVec{};

	glm::vec3 m_vel{ 0.0f, 0.0f, 0.0f };

	float airTime{ 0.3f };
};

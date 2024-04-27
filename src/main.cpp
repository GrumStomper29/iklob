#include "entity_system/entity.hpp"
#include "renderer/renderer.hpp"
#include "input/input.hpp"
#include "entity_system/camera.hpp"

#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include <PxPhysicsAPI.h>
#define SDL_MAIN_HANDLED
#include "SDL/sdl.h"

#include <cmath>
#include <iostream>
#include <string>
#include <unordered_map>
#include <utility> // for std::pair

//temp
//#include "glm/gtx/string_cast.hpp"

struct PhysicsState
{
	physx::PxDefaultAllocator defaultAllocatorCallback{};
	physx::PxDefaultErrorCallback defaultErrorCallback{};

	physx::PxDefaultCpuDispatcher* dispatcher{};
	physx::PxTolerancesScale toleranceScale{};
	physx::PxCooking* cooking{};

	physx::PxFoundation* foundation{};
	physx::PxPhysics* physics{};

	physx::PxScene* scene{};
	physx::PxControllerManager* controllerManager{};
	physx::PxMaterial* defaultMaterial{};

	physx::PxPvdTransport* transport{};
	physx::PxPvd* pvd{};
	physx::PxPvdSceneClient* pvdClient{};
};

// Verify no temps are created here that should be in PhysicsState
void initPhysicsState(PhysicsState& physicsState)
{
	physicsState.foundation = PxCreateFoundation(PX_PHYSICS_VERSION, 
		physicsState.defaultAllocatorCallback, physicsState.defaultErrorCallback);
	if (!physicsState.foundation)
	{
		// Todo: standardize errors
		std::cerr << "Error: PxCreateFoundation failed\n";
	}

	physicsState.pvd = physx::PxCreatePvd(*physicsState.foundation);
	physicsState.transport = physx::PxDefaultPvdSocketTransportCreate("", 5425, 10);
	physicsState.pvd->connect(*physicsState.transport, physx::PxPvdInstrumentationFlag::eALL);

	physicsState.toleranceScale.length = 1.0f;
	physicsState.toleranceScale.speed = 9.81f;

	physicsState.physics = PxCreatePhysics(PX_PHYSICS_VERSION, *physicsState.foundation, 
		physicsState.toleranceScale, true, physicsState.pvd);

	physicsState.dispatcher = physx::PxDefaultCpuDispatcherCreate(1);

	physicsState.cooking = PxCreateCooking(PX_PHYSICS_VERSION, *physicsState.foundation, 
		physx::PxCookingParams{ physicsState.toleranceScale });

	physx::PxSceneDesc sceneDesc{ physicsState.physics->getTolerancesScale() };
	sceneDesc.gravity = physx::PxVec3(0.0f, -9.81f, 0.0f);
	sceneDesc.cpuDispatcher = physicsState.dispatcher;
	sceneDesc.filterShader = physx::PxDefaultSimulationFilterShader;
	physicsState.scene = physicsState.physics->createScene(sceneDesc);

	physicsState.controllerManager = PxCreateControllerManager(*physicsState.scene);

	physicsState.pvdClient = physicsState.scene->getScenePvdClient();
	if (physicsState.pvdClient)
	{
		physicsState.pvdClient->setScenePvdFlag(physx::PxPvdSceneFlag::eTRANSMIT_CONSTRAINTS, true);
		physicsState.pvdClient->setScenePvdFlag(physx::PxPvdSceneFlag::eTRANSMIT_CONTACTS, true);
		physicsState.pvdClient->setScenePvdFlag(physx::PxPvdSceneFlag::eTRANSMIT_SCENEQUERIES, true);
	}

	physicsState.defaultMaterial = physicsState.physics->createMaterial(0.5f, 0.5f, 0.6f);
}

int main()
{
	SDL_Init(SDL_INIT_VIDEO);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 6);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
	SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 1);
	SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, 16);
	SDL_Window* window{ SDL_CreateWindow("iklob", 100, 100, 1600, 900, SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE) };
	SDL_GLContext glContext{ SDL_GL_CreateContext(window) };
	SDL_GL_MakeCurrent(window, glContext);

	Input input{};

	Renderer renderer{};
	renderer.init();
	renderer.setViewport(window);

	constexpr int modelCount{ 3 };
	std::pair<std::string, std::string> modelPaths[modelCount]
	{
		{ "assets/demo.glb", "level" },
		{ "assets/zombie.glb", "zombie" },
		{ "assets/gun.glb", "gun" }
	};

	renderer.loadScene(modelCount, modelPaths);

	glm::mat4 zombieTransform = glm::translate(glm::mat4{ 1.0f }, glm::vec3{0.0f, 1.0f, 0.0f});

	glm::mat4 gunTransform = glm::translate(glm::mat4{ 1.0f }, glm::vec3{0.0f, 2.0f, 0.0f});
	gunTransform = glm::scale(gunTransform, glm::vec3{ 0.2f });

	std::unordered_map<std::string, Entity> entities
	{
		{ "level", Entity{ { Entity::MeshID{ glm::mat4{ 1.0f }, "level" }}}},
		{ "zombie", Entity{ zombieTransform, { Entity::MeshID{ glm::mat4{ 1.0f }, "zombie" } } } },
		{ "gun", Entity{ glm::mat4{ 1.0f }, { Entity::MeshID{ gunTransform, "gun" } } } },
	};

	PhysicsState physicsState{};
	initPhysicsState(physicsState);

	physx::PxShape* groundBox{ physicsState.physics->createShape(
		physx::PxBoxGeometry{ 6.0f, 1.0f, 6.0f }, *physicsState.defaultMaterial) };
	physx::PxRigidStatic* groundRigid{ 
		physicsState.physics->createRigidStatic(physx::PxTransform{ physx::PxVec3{ 0.0f, 0.0f, 0.0f } }) };
	groundRigid->attachShape(*groundBox);
	physicsState.scene->addActor(*groundRigid);
	
	physx::PxShape* boxBox{ physicsState.physics->createShape(physx::PxBoxGeometry{ 1.0f, 1.0f, 1.0f }, 
		*physicsState.defaultMaterial) };
	physx::PxRigidStatic* boxRigid{ physicsState.physics->createRigidStatic(
		physx::PxTransform{ physx::PxVec3{ 4.0f, 2.0f, -4.0f } }) };
	boxRigid->attachShape(*boxBox);
	physicsState.scene->addActor(*boxRigid);
	
	Camera camera{ { 0.0f, 6.0f, 2.0f, }, physicsState.controllerManager, physicsState.defaultMaterial };

	glm::vec3 zombiePos{ 0.0f, 1.0f, 0.0f };
	float zombieAngle{ 0.0f };
	bool zombieDead{ false };

	float shootTime{ -10000.0 };

	bool quit{ false };

	double accumulator{ 0.0 };
	double lastTime{ SDL_GetTicks64() * 0.001 };
	bool drawn{ false };

	constexpr double deltaTime{ 1.0 / 60.0 };

	while (!quit)
	{
		double currentTime{ SDL_GetTicks64() * 0.001 };
		accumulator += currentTime - lastTime;
		lastTime = currentTime;

		glm::vec3 lightColor{ 1.0f, 1.0f, 0.71f };
		if (currentTime - shootTime < 0.25f)
		{
			// Lazy muzzle flash/blood effect
			lightColor = glm::mix(glm::vec3{ 1.0f, 0.5f, 0.0f }, glm::vec3{ 1.0f, 1.0f, 0.71f }, 
				static_cast<float>(currentTime - shootTime) * 4.0f);
		}

		while (accumulator > deltaTime)
		{
			for (auto& mesh : renderer.meshes)
			{
				// Only play animations if the zombie is still alive.
				// This is fine since the zombie is the only animated
				// mesh.
				if (!zombieDead)
				{
					mesh.second.time += accumulator;
					if (mesh.second.time > mesh.second.maxTime)
					{
						mesh.second.time = 0.0;
					}
				}
			}

			SDL_SetRelativeMouseMode(SDL_TRUE);

			SDL_Event e{};
			while (SDL_PollEvent(&e) != 0)
			{
				if (e.type == SDL_QUIT)
				{
					quit = true;
				}
				else if (e.type == SDL_MOUSEMOTION)
				{
					camera.m_yaw += e.motion.xrel * static_cast<float>(deltaTime) * camera.m_lookSpeed;
					camera.m_pitch -= e.motion.yrel * static_cast<float>(deltaTime) * camera.m_lookSpeed;

					if (camera.m_pitch > 89.0f)
					{
						camera.m_pitch = 89.0f;
					}
					else if (camera.m_pitch < -89.0f)
					{
						camera.m_pitch = -89.0f;
					}
				}
				else if (e.type == SDL_MOUSEBUTTONDOWN)
				{
					shootTime = currentTime;

					glm::vec3 zombieHitboxPos{ zombiePos.x, zombiePos.y + 2.0f, zombiePos.z };
					double zombieHitboxRadius{ 1.0f };

					// Trace ray against zombie hitbox to detect if shot lands
					const glm::vec3 m{ 
						glm::vec3{ camera.getPos().x, camera.getPos().y, camera.getPos().z } - zombieHitboxPos };
					const double b{ glm::dot(m, camera.getForwardVec()) };
					const double c{ glm::dot(m, m) - (zombieHitboxRadius * zombieHitboxRadius) };
					if (!(c > 0.0 && b > 0.0))
					{
						const double discr{ b * b - c };

						if (!(discr < 0.0))
						{
							zombieDead = true;
						}
					}
				}
				else
				{
					input.update(e);
				}
			}

			if (!zombieDead)
			{
				if (camera.getPos().z >= zombiePos.z)
				{
					zombieAngle = std::atan((camera.getPos().x - zombiePos.x) / (camera.getPos().z - zombiePos.z));
				}
				else
				{
					zombieAngle = (std::atan(1.0f) * 4.0f) + std::atan((camera.getPos().x - zombiePos.x) / (camera.getPos().z - zombiePos.z));
				}
				zombiePos.x += std::sin(zombieAngle) * deltaTime;
				zombiePos.z += std::cos(zombieAngle) * deltaTime;

				if (glm::distance(glm::vec3{ camera.getPos().x, camera.getPos().y, camera.getPos().z }, zombiePos) < 3.0f)
				{
					// Doing this makes the screen red
					shootTime = currentTime + 1.0f;
					// Bounce player back
					camera.setVel({ -camera.getForwardVec().x * 5.0f, camera.getVel().y, -camera.getForwardVec().z * 5.0f });
				}
			}
			else
			{
				// Sink zombie into ground
				zombiePos.y -= 1.0f * deltaTime;
			}

			zombieTransform = glm::translate(glm::mat4{ 1.0f }, zombiePos);
			zombieTransform = glm::rotate(zombieTransform, zombieAngle, glm::vec3{ 0.0f, 1.0f, 0.0f });
			entities.at("zombie").setTransform(zombieTransform);

			camera.calculateFrontVec();
			camera.update(input, deltaTime);
			physicsState.scene->simulate(deltaTime);
			physicsState.scene->fetchResults(true);

			glm::vec3 gunPos{ camera.getPos().x, camera.getPos().y, camera.getPos().z };
			glm::mat4 gunTransform{ glm::translate(glm::mat4{ 1.0f }, gunPos) };
			gunTransform = glm::rotate(gunTransform, glm::radians(-camera.m_yaw + 180), glm::vec3{ 0.0f, 1.0f, 0.0f });
			gunTransform = glm::rotate(gunTransform, glm::radians(-camera.m_pitch), glm::vec3{ 0.0f, 0.0f, 1.0f });
			gunTransform = { glm::translate(gunTransform, glm::vec3{ -0.15f, -0.55f, 0.1f }) }; // Put gun in left hand
			gunTransform = glm::scale(gunTransform, glm::vec3{ 0.2f });
			entities.at("gun").setTransform(gunTransform);

			accumulator -= deltaTime;
			drawn = false;
		}

		if (!drawn)
		{
			const auto& pxCamPos{ camera.getPos() };
			glm::vec3 cameraPosition{ pxCamPos.x, pxCamPos.y, pxCamPos.z };

			renderer.setViewport(window);
			renderer.beginRendering(cameraPosition, camera.getForwardVec(),
				90.0f, 16.0f / 9.0f, 0.1f, 500.0f, lightColor);

			for (const auto& entity : entities)
			{
				entity.second.render([&](Entity::MeshID m, const glm::mat4& tr)
					{
						renderer.renderMesh(m.second, tr * m.first);
					});
			}

			SDL_GL_SwapWindow(window);
			drawn = true;
		}
	}

	physicsState.scene->release();
	physicsState.physics->release();
	physicsState.foundation->release();

	renderer.cleanup();

	SDL_DestroyWindow(window);

	return 0;
}
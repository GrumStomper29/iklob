#include "renderer/renderer.hpp"
#include "input/input.hpp"
#include "physics/camera.hpp"
#include "physics/collision_detection.hpp"

#include "glm/glm.hpp"
#define SDL_MAIN_HANDLED
#include "SDL/sdl.h"

#include <cmath>
#include <string>

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

	std::string modelPaths[2]
	{
		"assets/test_level.glb",
		"assets/zombie1.glb",
	};
	renderer.loadScene(2, modelPaths);

	Camera camera
	{
		.position{ 0.0f, 10.0f, 10.0f },
		.moveSpeed{ 2.5f }
	};
	calculateCameraFrontVector(camera);
	
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

		while (accumulator > deltaTime)
		{
			for (auto& mesh : renderer.m_meshes)
			{
				for (auto& primitive : mesh.primitives)
				{
					calculatePrimitiveBounds(renderer, primitive);
				}
			}

			for (auto& mesh : renderer.m_meshes)
			{
				mesh.time += accumulator;
				if (mesh.time > mesh.maxTime)
				{
					mesh.time = 0.0;
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
					camera.yaw += e.motion.xrel * static_cast<float>(deltaTime) * camera.lookSpeed;
					camera.pitch -= e.motion.yrel * static_cast<float>(deltaTime) * camera.lookSpeed;

					if (camera.pitch > 89.0f)
					{
						camera.pitch = 89.0f;
					}
					else if (camera.pitch < -89.0f)
					{
						camera.pitch = -89.0f;
					}
				}
				else
				{
					input.update(e);
				}
			}

			updateCamera(camera, input, renderer, static_cast<float>(deltaTime));

			accumulator -= deltaTime;
			drawn = false;
		}

		if (drawn)
		{
			// Interpolate physics
			// Todo: what goes here
		}
		else
		{
			renderer.setViewport(window);
			renderer.render(camera.position, camera.forwardVector,
				90.0f, 16.0f / 9.0f, 0.1f, 500.0f);
			SDL_GL_SwapWindow(window);

			drawn = true;
		}
	}

	renderer.cleanup();

	SDL_DestroyWindow(window);

	return 0;
}
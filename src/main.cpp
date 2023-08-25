#include "renderer/renderer.hpp"

#include "input/input.hpp"

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

	std::string modelPaths[1]
	{
		"assets/Deccer_Cubes.glb"
	};
	renderer.loadScene(1, modelPaths);

	glm::vec3 cameraPosition{ 0.0f, 0.0f, 10.0f };
	float cameraYaw{ -90.0f };
	float cameraPitch{ 0.0f };

	glm::vec3 cameraFront{
		std::cos(glm::radians(cameraYaw))* std::cos(glm::radians(cameraPitch)),
		std::sin(glm::radians(cameraPitch)),
		std::sin(glm::radians(cameraYaw))* std::cos(glm::radians(cameraPitch)) };

	constexpr float cameraLookSpeed{ 10.0f };
	constexpr float cameraMoveSpeed{ 10.0f };

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
					cameraYaw += e.motion.xrel * deltaTime * cameraLookSpeed;
					cameraPitch -= e.motion.yrel * deltaTime * cameraLookSpeed;
				}
				else
				{
					input.update(e);
				}
			}

			cameraFront = {
				std::cos(glm::radians(cameraYaw))* std::cos(glm::radians(cameraPitch)),
				std::sin(glm::radians(cameraPitch)),
				std::sin(glm::radians(cameraYaw))* std::cos(glm::radians(cameraPitch)) };

			glm::vec3 cameraRight{ glm::normalize(glm::cross(glm::vec3{ 0.0f, 1.0f, 0.0f }, cameraFront)) };

			float cameraRightVel{ input.getKeyDown(Input::D) ? cameraMoveSpeed * static_cast<float>(deltaTime) : (input.getKeyDown(Input::A) ? -cameraMoveSpeed * static_cast<float>(deltaTime) : 0.0f) };
			float cameraForwardVel{ input.getKeyDown(Input::W) ? -cameraMoveSpeed * static_cast<float>(deltaTime) : (input.getKeyDown(Input::S) ? cameraMoveSpeed * static_cast<float>(deltaTime) : 0.0f) };

			cameraPosition -= cameraFront * cameraForwardVel;
			cameraPosition -= cameraRight * cameraRightVel;

			accumulator -= deltaTime;
			drawn = false;
		}

		if (drawn)
		{
			// Interpolate
		}
		else
		{
			renderer.setViewport(window);
			renderer.render(cameraPosition, cameraFront,
				90.0f, 16.0f / 9.0f, 0.1f, 500.0f);
			SDL_GL_SwapWindow(window);

			drawn = true;
		}
	}

	renderer.cleanup();

	SDL_DestroyWindow(window);

	return 0;
}
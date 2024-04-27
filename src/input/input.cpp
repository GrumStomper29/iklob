#include "input.hpp"

#define SDL_MAIN_HANDLED
#include "SDL/SDL.h"

void Input::update(const SDL_Event& e)
{
	if (e.type == SDL_KEYDOWN || e.type == SDL_KEYUP)
	{
		switch (e.key.keysym.sym)
		{
		case SDLK_a:
			keyStates[A] = (e.type == SDL_KEYDOWN);
			break;
		case SDLK_d:
			keyStates[D] = (e.type == SDL_KEYDOWN);
			break;
		case SDLK_s:
			keyStates[S] = (e.type == SDL_KEYDOWN);
			break;
		case SDLK_w:
			keyStates[W] = (e.type == SDL_KEYDOWN);
			break;
		case SDLK_SPACE:
			keyStates[SPACE] = (e.type == SDL_KEYDOWN);
			break;
		}
	}
}

bool Input::getKeyDown(KeyCode key) const
{
	return keyStates[key];
}
#pragma once

#define SDL_MAIN_HANDLED
#include "SDL/SDL.h"

class Input
{
public:

	enum KeyCode
	{
		A,
		D,
		S,
		W
	};

	void update(const SDL_Event& e);

	bool getKeyDown(KeyCode key);

private:

	bool keyStates[4]{};
};
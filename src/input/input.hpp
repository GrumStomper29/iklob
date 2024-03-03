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
		W,
		SPACE,
	};

	void update(const SDL_Event& e);

	bool getKeyDown(KeyCode key) const;

private:

	bool keyStates[5]{};
};
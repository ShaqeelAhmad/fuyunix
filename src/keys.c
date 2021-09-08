/*
 *  Copyright 2021 Shaqeel Ahmad
 *
 *  This file is part of fuyunix.
 *
 *  fuyunix is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  fuyunix is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with fuyunix.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <stdbool.h>
#include <stdio.h>
#include <SDL.h>

#include "drw.h"
#include "fuyunix.h"

/* Global variables */
static SDL_KeyboardEvent *keyevent;

/* Function declarations */
struct Key {
	SDL_Scancode key;
	void (*func)(int);
	int player;
};

static const struct Key key[] = {
	{SDL_SCANCODE_Q,          quitloop,     0},
	{SDL_SCANCODE_ESCAPE,     drwMenu,     0},

	/* Player 1 */
	{SDL_SCANCODE_H,             left,     0},
	{SDL_SCANCODE_J,             down,     0},
	{SDL_SCANCODE_K,             jump,     0},
	{SDL_SCANCODE_L,            right,     0},

	/* Player 2 */
	{SDL_SCANCODE_A,             left,     1},
	{SDL_SCANCODE_S,             down,     1},
	{SDL_SCANCODE_W,             jump,     1},
	{SDL_SCANCODE_D,            right,     1},
};

static void
loopkeys(void)
{
	size_t i;
	size_t size = sizeof(key) / sizeof(key[0]);
	for (i = 0; i < size; i++) {
		if (keyevent->keysym.scancode == key[i].key) {
			key[i].func(key[i].player);
		}

		const Uint8 *state = SDL_GetKeyboardState(NULL);
		if (left == key[i].func && state[key[i].key])
			key[i].func(key[i].player);
		if (right == key[i].func && state[key[i].key])
			key[i].func(key[i].player);
		if (down == key[i].func && state[key[i].key])
			key[i].func(key[i].player);
		if (jump == key[i].func && state[key[i].key])
			key[i].func(key[i].player);
	}
}

int
handleMenuKeys(int *focus, int last)
{
	SDL_Event event;

	while (SDL_PollEvent(&event) != 0) {
		if (event.type == SDL_QUIT) {
			quitloop();
			*focus = last;
			return 1;
		} else if (event.type == SDL_KEYDOWN) {
			switch (event.key.keysym.scancode) {
			case SDL_SCANCODE_K:
				if (*focus > 0)
					*focus -= 1;
				break;
			case SDL_SCANCODE_J:
				if (*focus < last)
					*focus += 1;
				break;
			case SDL_SCANCODE_Q:
				quitloop();
				*focus = last;
				return 1;
			case SDL_SCANCODE_RETURN:
			case SDL_SCANCODE_SPACE:
				return 1;
				// gcc / clang needs default:
			default:
				break;
			}
		}
	}
	return 0;
}

void
handleKeys(void *ptr)
{
	keyevent = ptr;

	loopkeys();
}

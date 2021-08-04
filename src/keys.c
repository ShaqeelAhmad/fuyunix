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
#include "file.h"

/* Global variables */
SDL_KeyboardEvent *keyevent;

/* Function declarations */
void left(int player);
void right(int player);
void down(int player);
void jump(int player);

struct Key {
	SDL_Scancode key;
	void (*func)(int);
	int player;
};

static const struct Key key[] = {
	{SDL_SCANCODE_Q,         quitloop,     0},
	{SDL_SCANCODE_ESCAPE,     drwmenu,     0},

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

void
left(int player)
{
	fprintf(stderr, "%d\n", player);
}

void
down(int player)
{
	fprintf(stderr, "%d\n", player);
}

void
jump(int player)
{
	fprintf(stderr, "%d\n", player);
}

void
right(int player)
{
	fprintf(stderr, "%d\n", player);
}

void
loopkeys(void)
{
	/* TODO handle the key differently (SCANCODE ?) */
	unsigned long int i;
	size_t size = sizeof(key) / sizeof(key[0]);
	for (i = 0; i < size; i++) {
		if (keyevent->keysym.scancode == key[i].key) {
			key[i].func(key[i].player);
		}
	}
}

void
handleKeys(void *ptr)
{
	keyevent = ptr;

	loopkeys();
}

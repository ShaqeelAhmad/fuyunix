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

struct List list[] = {
	{0, "quitloop",      "Q"},
	{0, "left",          "H"},
	{0, "drawmenu", "Escape"},
};

/*
struct Funcname {
	void (*func)(int);
	char *name;
};

static struct Funcname funcname[] = {
	{quitloop,   "quitloop"},
	{drwmenu,    "drawmenu"},
	{left,      "move-left"},
	{down,     "move-down?"},
	{jump,           "jump"},
	{right,    "move-right"},
};
*/

struct Key {
	SDL_Keycode key;
	void (*func)(int);
	int player;
};

/*
struct value {
	char *name;
	int player;
	char *funcname;
	void (*func)(int);
};

if (strcmp(value.name, foo.funcname) == 0) {
	value.func = foo.func;
}

*/

/* Default keys */
static const struct Key key[] = {
	{SDLK_q,         quitloop,     0},
	{SDLK_ESCAPE,     drwmenu,     0},

	/* Player 1 */
	{SDLK_h,             left,     0},
	{SDLK_j,             down,     0},
	{SDLK_k,             jump,     0},
	{SDLK_l,            right,     0},

	/* Player 2 */
	{SDLK_a,             left,     1},
	{SDLK_s,             down,     1},
	{SDLK_w,             jump,     1},
	{SDLK_d,            right,     1},
};

void
saveKeys()
{
	writeKeysFile(list, sizeof(list) / sizeof(list[0]));
}

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
	/* TODO handle the key differently */
	unsigned long int i;
	size_t size = sizeof(key) / sizeof(key[0]);
	for (i = 0; i < size; i++) {
		if (keyevent->keysym.sym == key[i].key) {
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

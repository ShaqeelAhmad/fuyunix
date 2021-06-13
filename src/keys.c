/* 
 * This file is only meant for keys and functions that get executed by the
 * keys. The functions may be from other files such as drw.c
*/
#include <stdio.h>
#include <stdbool.h>
#include <SDL2/SDL.h>

#include "drw.h"

SDL_KeyboardEvent *keyevent;

void quitloop(void);

struct Key {
	SDL_Keycode key;
	void (*func)(void);
};

static const struct Key key[] = {
	{SDLK_a,         quitloop},
	{SDLK_h,         quitloop},
	{SDLK_j,         quitloop},
	{SDLK_k,         quitloop},
	{SDLK_l,         quitloop},
	{SDLK_ESCAPE,     drwmenu},
};

bool running;

void
quitloop(void)
{
	running = false;
}

void
loopkeys(void)
{
	unsigned long int i;
	size_t size = sizeof(key) / sizeof(key[0]);
	for (i = 0; i < size; i++) {
		if (keyevent->keysym.sym == key[i].key) {
			key[i].func();
		}
	}
}

bool
handleKeys(void *ptr)
{
	keyevent = ptr;

	running = true;

	loopkeys();

	return !running;
}

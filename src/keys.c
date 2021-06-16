/* 
 * This file is only meant for keys and functions that get executed by the
 * keys. The functions may be from other files such as drw.c
*/
#include <stdio.h>
#include <stdbool.h>
#include <SDL2/SDL.h>

#include "drw.h"
#include "fuyunix.h"

SDL_KeyboardEvent *keyevent;

/* Function declarations */
void left(void);
void right(void);
void down(void);
void jump(void);

struct Key {
	SDL_Keycode key;
	void (*func)(void);
};

/* Default keys */
static const struct Key key[] = {
	{SDLK_q,         quitloop},
	{SDLK_h,             left},
	{SDLK_j,             down},
	{SDLK_k,             jump},
	{SDLK_l,            right},
	{SDLK_ESCAPE,     drwmenu},
};

void
left(void)
{
}

void
down(void)
{
}

void
jump(void)
{
}

void
right(void)
{
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

void
handleKeys(void *ptr)
{
	keyevent = ptr;

	loopkeys();
}

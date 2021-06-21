#include <stdbool.h>
#include <stdio.h>
#include <SDL2/SDL.h>

#include "drw.h"
#include "fuyunix.h"

/* Global variables */
SDL_KeyboardEvent *keyevent;

/* Function declarations */
void left(int player);
void right(int player);
void down(int player);
void jump(int player);

struct Key {
	SDL_Keycode key;
	void (*func)(int);
	int player;
};

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

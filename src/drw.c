#include <stdio.h>
#include <SDL2/SDL.h>
#include "fuyunix.h"

/* Global struct */
static struct Game game;

void
init(void)
{
	SDL_Init(SDL_INIT_VIDEO);

	SDL_GetDesktopDisplayMode(0, &game.dm);

	game.win = SDL_CreateWindow(GAMENAME,
			SDL_WINDOWPOS_UNDEFINED,
			SDL_WINDOWPOS_UNDEFINED,
			game.dm.w,
			game.dm.h,
			0);

	game.rnd = SDL_CreateRenderer(game.win, -1, 0);
}

void 
cleanup(void)
{
	SDL_DestroyRenderer(game.rnd);
	SDL_DestroyWindow(game.win);

	SDL_Quit();
}

void
drwmenu(void)
{
	fprintf(stderr, "%d\n", game.dm.w);
}

void 
drw(void)
{
	fprintf(stderr, "%s\n", resourcepath);

}

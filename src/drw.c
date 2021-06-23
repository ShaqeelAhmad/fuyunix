#include <stdio.h>
#include <stdbool.h>
#include <SDL2/SDL.h>

#include "fuyunix.h"

/* structs */
struct Game {
	SDL_Window *win;
	SDL_Renderer *rnd;

	SDL_Surface *surf;

	SDL_DisplayMode dm;

	int h;
	int w;
};

static struct Game game;

/* Function definitions */
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
drwmenu(int player)
{
	int x = 20;
	int y = 20;
	int width = game.w - x * 2;
	int height = game.h - y * 2;
	SDL_Rect menu = {
		x,
		y,
		width,
		height
	};

	SDL_FillRect(game.surf, &menu,
			SDL_MapRGB(game.surf->format, 20, 150, 180)
			);

	SDL_RenderPresent(game.rnd);
}

void
drw(void)
{
	game.surf = SDL_GetWindowSurface(game.win);
	SDL_GetWindowSize(game.win, &game.w, &game.h);

	SDL_UpdateWindowSurface(game.win);
}

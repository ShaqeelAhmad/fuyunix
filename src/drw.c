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

#include <stdio.h>
#include <stdbool.h>
#include <SDL.h>
#include <SDL_image.h>

#include "fuyunix.h"

/* structs */
struct Player {
	SDL_Texture *frame[4];

	int x;
	int y;
};

struct Game {
	SDL_Window *win;
	SDL_Renderer *rnd;

	SDL_Surface *surf;

	SDL_DisplayMode dm;

	int h;
	int w;
	int numplayers;
};

static struct Game game;

void drw(void);

/* Function definitions */
/* TODO Draw text */
void
homeMenu(void)
{
	SDL_Event event;

	game.numplayers = 0;

	int focus = 0;
	bool notquit = true;

	/* height of choices */
	int ch;
	int oldheight = 0;
	int oldwidth = 0;
	int winwidth;
	int winheight;
	int diff = 30;

selection_loop:
	while (notquit) {
		while (SDL_PollEvent(&event) != 0) {
			if (event.type == SDL_QUIT) {
				notquit = false;
				quitloop(0);
				return;
			} else if (event.type == SDL_KEYDOWN) {
				switch (event.key.keysym.scancode) {
					case SDL_SCANCODE_K:
						if (focus > 0)
							focus -= 1;
						break;
					case SDL_SCANCODE_J:
						if (focus < 2)
							focus += 1;
						break;
					case SDL_SCANCODE_Q:
						notquit = false;
						quitloop(0);
						return;
					case SDL_SCANCODE_RETURN:
						notquit = false;
						break;
					default:
						break;
				}
			}
		}

		/* TODO add game name on top part of screen */

		SDL_GetWindowSize(game.win, &game.w, &game.h);
		game.surf = SDL_GetWindowSurface(game.win);

		if (game.h != oldheight) {
			ch = game.h >> 2;
			oldheight = game.h;
			winheight = ch - (diff << 1);
		}
		if (game.w != oldwidth) {
			winwidth = game.w - (diff << 1);
			oldwidth = game.w;
		}

		SDL_Rect options[3] = {
			{diff, diff + ch, winwidth, winheight},
			{diff, diff + (ch << 1), winwidth, winheight},
			{diff, diff + (ch << 1) + ch, winwidth, winheight},
		};

		SDL_FillRects(game.surf, options, 3,
				SDL_MapRGB(game.surf->format, 20, 150, 180));
		SDL_FillRect(game.surf, &options[focus],
				SDL_MapRGB(game.surf->format, 20, 190, 180));

		SDL_RenderPresent(game.rnd);

		SDL_UpdateWindowSurface(game.win);

	}

	if (focus == 1) {
		if (game.numplayers == 0)
			game.numplayers = 1;
		else
			game.numplayers = 0;

		printf("Num players = %d\n", game.numplayers);
		notquit = true;
		goto selection_loop;
	} else if (focus == 2) {
		printf("Exit\n");
		quitloop(0);
	}
}

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
	/* TODO Possibly write savefile here */
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
			SDL_MapRGB(game.surf->format, 20, 150, 180));

	SDL_RenderPresent(game.rnd);
}

void
drw(void)
{
	SDL_GetWindowSize(game.win, &game.w, &game.h);
	game.surf = SDL_GetWindowSurface(game.win);

	/* printf("%d\n", game.w); */
	/* printf("%d\n", game.h); */

	SDL_UpdateWindowSurface(game.win);
}

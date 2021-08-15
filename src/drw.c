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

#include <cairo.h>
#include <stdio.h>
#include <stdbool.h>
#include <SDL.h>
#include <SDL_image.h>

#include "fuyunix.h"
#include "file.h"

/* static const char resourcepath[] = "/usr/local/games/fuyunix/"; */

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

	int h;
	int w;

	/* Old width / height */
	int ow;
	int oh;

	int numplayers;
	int level;
};

static struct Game game;

/* I'm lazy to manually allocate memory and check errors */
static struct Player player[2];

/* Function definitions */
static bool
handleMenuKeys(int *focus, int max)
{
	SDL_Event event;

	while (SDL_PollEvent(&event) != 0) {
		if (event.type == SDL_QUIT) {
			quitloop();
			*focus = 2;
			return false;
		} else if (event.type == SDL_KEYDOWN) {
			switch (event.key.keysym.scancode) {
				case SDL_SCANCODE_K:
					if (*focus > 0)
						*focus -= 1;
					break;
				case SDL_SCANCODE_J:
					if (*focus < max)
						*focus += 1;
					break;
				case SDL_SCANCODE_Q:
					quitloop();
					*focus = 2;
					return false;
				case SDL_SCANCODE_RETURN:
				case SDL_SCANCODE_SPACE:
					return false;
					/* gcc / clang needs default: */
				default:
					break;
			}
		}
	}
	return true;
}

static void
drwMenuText(char *text, int x, int y, double size)
{
	cairo_surface_t *cSurface = cairo_image_surface_create_for_data(
			(unsigned char *)game.surf->pixels, CAIRO_FORMAT_RGB24,
			game.surf->w, game.surf->h, game.surf->pitch);

	cairo_t *cr = cairo_create(cSurface);

	cairo_select_font_face(cr, "normal", CAIRO_FONT_SLANT_NORMAL,
			CAIRO_FONT_WEIGHT_NORMAL);
	cairo_set_font_size(cr, size);
	cairo_set_source_rgb(cr, 1.0, 1.0, 1.0);
	cairo_move_to(cr, (double)x, (double)y);
	cairo_show_text(cr, text);

	cairo_destroy(cr);
	cairo_surface_destroy(cSurface);
}

void
homeMenu(void)
{
	int focus = 0;
	bool notquit = true;

	int diff = 30;
	int ch;
	int winwidth;
	int winheight;

selection_loop:
	while (notquit) {
		notquit = handleMenuKeys(&focus, 2);

		SDL_GetWindowSize(game.win, &game.w, &game.h);

		if (game.h != game.oh) {
			ch = game.h >> 2;
			winheight = ch - (diff << 1);
			game.oh = game.h;
			game.surf = SDL_GetWindowSurface(game.win);
		}
		if (game.w != game.ow) {
			winwidth = game.w - (diff << 1);
			game.ow = game.w;
			game.surf = SDL_GetWindowSurface(game.win);
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

		/* Clang requires array to be of size 2 */
		char nplayer[2];
		nplayer[0] = game.numplayers + '1';

		/* TODO Calculate where to position text */
		drwMenuText(NAME, 0, winheight >> 1, 64.0);

		drwMenuText("Start", 10 + diff, 75 + diff + ch, 32.0);
		drwMenuText("Choose players", 10 + diff, 75 + diff + (ch << 1), 32.0);
		drwMenuText(nplayer, winwidth - diff, 75 + diff + (ch << 1), 32.0);
		drwMenuText("Exit", 10 + diff, 75 + diff + (ch << 1) + ch, 32.0);

		SDL_UpdateWindowSurface(game.win);
		SDL_Delay(16);
	}

	if (focus == 1) {
		if (game.numplayers == 0)
			game.numplayers = 1;
		else
			game.numplayers = 0;

		notquit = true;
		goto selection_loop;
	} else if (focus == 2) {
		printf("Exit\n");
		quitloop();
	}
}

static void
initVariables()
{
	game.level = readSaveFile();

	game.numplayers = 0;

	game.ow = 0;
	game.oh = 0;

	player[0].x = 0;
	player[0].y = 0;
	player[1].x = 0;
	player[1].y = 0;
}

void
init(void)
{
	SDL_Init(SDL_INIT_VIDEO);

	game.win = SDL_CreateWindow(NAME, SDL_WINDOWPOS_UNDEFINED,
			SDL_WINDOWPOS_UNDEFINED, 640, 480, 0);

	game.rnd = SDL_CreateRenderer(game.win, -1, 0);


	initVariables();
}

void
cleanup(void)
{
	writeSaveFile(game.level);

	SDL_DestroyRenderer(game.rnd);
	SDL_DestroyWindow(game.win);

	SDL_Quit();
}

void
drwmenu(int player)
{
	/* TODO Make an actual menu. I probably won't do this soon */

	SDL_FillRect(game.surf, NULL,
			SDL_MapRGB(game.surf->format, 0, 0, 0));
	drwMenuText("Use your window manager to quit", 0, game.h - 20, 40);

	SDL_UpdateWindowSurface(game.win);
}

static void
drwplayers(void)
{
	/* player[p].frame = IMG_Load("../data/sprites/players/player1.png"); */
}

static void
getSurf(void)
{
	SDL_GetWindowSize(game.win, &game.w, &game.h);
	if (game.w != game.ow) {
		game.ow = game.w;
		game.surf = SDL_GetWindowSurface(game.win);
	}
	if (game.h != game.oh) {
		game.oh = game.h;
		game.surf = SDL_GetWindowSurface(game.win);
	}
}


void
drw(void)
{
	getSurf();

	drwplayers();

	SDL_UpdateWindowSurface(game.win);
}

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
#include "drw.h"

/* structs */
struct Player {
	SDL_Texture *frame[4];
	SDL_Texture **current;

	int x;
	int y;
	int w;
	int h;
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

/* Global variables */
static struct Game game;
static struct Player *player;


/* Function declarations */
static void freePlayerTextures();
static void loadPlayerTextures();


/* Function definitions */
static void
initVariables()
{
	game.level = readSaveFile();

	game.numplayers = 0;

	game.ow = 0;
	game.oh = 0;

	player = NULL;
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
	freePlayerTextures();

	writeSaveFile(game.level);

	SDL_DestroyRenderer(game.rnd);
	SDL_DestroyWindow(game.win);

	SDL_Quit();
}

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

/*
 * Note: The bitshifts in this function are basically division or
 * multiplications and can be replaced with the equivalent versions
 */
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

		if (game.h != game.oh || game.w != game.ow) {
			ch = game.h >> 2;
			diff = game.h >> 6;
			winheight = ch - (diff << 1);
			game.oh = game.h;

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

		/* clang requires array to be of size 2 */
		char nplayer[2];
		nplayer[0] = game.numplayers + '1';
		nplayer[1] = '\0';

		/* TODO Calculate where to position text */
		/* Note: I'll probably do it when the game is almost finished */
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
	} else {
		loadPlayerTextures();
	}
}

void
drwmenu(int player)
{
	/* TODO Make an actual menu. I probably won't do this soon */

	quitloop();
}

static void
loadPlayerTextures(void)
{
	player = (struct Player *)calloc(game.numplayers + 1, sizeof(struct Player));
	if (player == NULL) {
		quitloop();
		return;
	}

	for (int i = 0; i <= game.numplayers; i++) {
		/* TODO Load all the frames for all the players. */
		player[i].frame[0] = IMG_LoadTexture(game.rnd,
				RESOURCE_PATH "/data/sprite.png");
	}
	player[0].current = &player[0].frame[0];

	player[0].x = 0;
	player[0].y = 0;

	/* Sprite is 32x32 pixels */
	/* possibly scale according to screen size */
	player[0].w = 32 * 2;
	player[0].h = 32 * 2;
}

static void
freePlayerTextures(void)
{
	if (player == NULL) {
		return;
	}

	for (int i = 0; i <= game.numplayers; i++) {
		SDL_DestroyTexture(player[i].frame[0]);
	}

	free(player);
}

static void
drwplayers(void)
{
	SDL_RenderClear(game.rnd);
	SDL_Rect playrect = {
		player[0].x,
		player[0].y,
		player[0].w,
		player[0].h,
	};

	if (SDL_RenderCopy(game.rnd, *player[0].current, NULL, &playrect) < 0) {
		fprintf(stderr, "%s\n", SDL_GetError());
	}
	SDL_RenderPresent(game.rnd);
}

void
left(int i)
{
	if (player[0].x <= 0)
		return;

	player[0].x -= 5;
}

void
down(int i)
{
	if ((player[0].y + player[0].h) >= game.h)
		return;

	player[0].y += 5;
}

void
jump(int i)
{
	/* TODO Actually implement jump and gravity */
	if (player[0].y <= 0)
		return;

	player[0].y -= 5;
}

void
right(int i)
{
	if ((player[0].x + player[0].w) >= game.w)
		return;

	player[0].x += 5;
}

static void
getSurf(void)
{
	SDL_GetWindowSize(game.win, &game.w, &game.h);
	if (game.w != game.ow || game.h != game.oh) {
		game.ow = game.w;
		game.oh = game.h;
		game.surf = SDL_GetWindowSurface(game.win);
	}
}


void
drw(void)
{
	getSurf();

	drwplayers();

	/* SDL_UpdateWindowSurface(game.win); */
}

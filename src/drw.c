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

#include "alloc.h"
#include "file.h"
#include "fuyunix.h"
#include "keys.h"

#define GRAVITY 0.035f

/* structs */
struct Player {
	SDL_Texture *frame[4];
	SDL_Texture **current;

	double x;
	double y;

	double dx;
	double dy;

	int w;
	int h;

	int falling;
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

	getKeys();

	initVariables();
}

void
cleanup(void)
{
	freeKeys();

	freePlayerTextures();

	writeSaveFile(game.level);

	SDL_DestroyRenderer(game.rnd);
	SDL_DestroyWindow(game.win);

	SDL_Quit();
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
	int ch = 0;
	int winwidth = 0;
	int winheight = 0;

selection_loop:
	while (notquit) {
		notquit = !handleMenuKeys(&focus, 2);

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
		drwMenuText(NAME, 0, winheight >> 1, 64.0);

		drwMenuText("Start", 10 + diff, 75 + diff + ch, 32.0);
		drwMenuText("Choose players", 10 + diff, 75 + diff + (ch << 1), 32.0);
		drwMenuText(nplayer, winwidth - diff, 75 + diff + (ch << 1), 32.0);
		drwMenuText("Exit", 10 + diff, 75 + diff + (ch << 1) + ch, 32.0);

		SDL_UpdateWindowSurface(game.win);
		SDL_Delay(16);
	}

	if (focus == 1) {
		if (game.numplayers >= 1)
			game.numplayers = 0;
		else
			game.numplayers++;

		notquit = true;
		goto selection_loop;
	} else if (focus == 2) {
		printf("Exit\n");
		quitloop();
		return;
	} else {
		loadPlayerTextures();
	}
}

void
drwMenu(int player)
{
	/* TODO Draw a menu */
	quitloop();
}

static void
loadPlayerTextures(void)
{
	player = (struct Player *)qcalloc(
			game.numplayers + 1, sizeof(struct Player));

	for (int i = 0; i <= game.numplayers; i++) {
		player[i].frame[0] = IMG_LoadTexture(game.rnd,
				RESOURCE_PATH "/data/sprite.png");

		if (player[i].frame[0] == NULL) {
			fprintf(stderr, "Unable to Load Texture: %s\n", IMG_GetError());
		}

		player[i].current = &player[0].frame[0];

		player[i].x = 0;
		player[i].y = 0;
		player[i].dx = 0;
		player[i].dy = 0;

		/* Sprite is 32x32 pixels */
		/* possibly scale according to screen size */
		player[i].w = 32 * 2;
		player[i].h = 32 * 2;
	}
}

static void
freePlayerTextures(void)
{
	/* Causes segfaults if ignored */
	if (player == NULL)
		return;

	for (int i = 0; i <= game.numplayers; i++) {
		SDL_DestroyTexture(player[i].frame[0]);
	}

	free(player);
}

static void
drwPlayers(void)
{
	for (int i = 0; i <= game.numplayers; i++) {
		SDL_Rect playrect = {player[i].x, player[i].y,
			player[i].w, player[i].h};

		/* Draw a white square in place of texture that failed to load */
		if (*player[i].current == NULL) {
			SDL_SetRenderDrawColor(game.rnd, 255, 255, 255, 255);
			SDL_RenderFillRect(game.rnd, &playrect);
			SDL_SetRenderDrawColor(game.rnd, 0, 0, 0, 255);
		} else {
			if (SDL_RenderCopy(game.rnd, *player[i].current,
						NULL, &playrect) < 0) {

				fprintf(stderr, "%s\n", SDL_GetError());
			}
		}
	}
}

void
down(int i)
{
	/* Do nothing for now */
}

void
jump(int i)
{
	i = i > game.numplayers ? 0 : i;

	if (player[i].y <= 0 || player[i].falling)
		return;

	player[i].dy = -5;

	player[i].falling = 1;
}

void
right(int i)
{
	i = i > game.numplayers ? 0 : i;

	if ((player[i].x + player[i].w) >= game.w)
		return;

	if (player[i].dx < 3)
		player[i].dx += 0.6;
}

void
left(int i)
{
	i = i > game.numplayers ? 0 : i;

	if (player[i].x <= 0)
		return;

	if (player[i].dx > -3)
		player[i].dx -= 0.6;
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

static void
drwGround(void)
{
	int groundSize = 20;
	SDL_Rect ground = {
		0, game.h - groundSize, game.w, groundSize,
	};
	SDL_SetRenderDrawColor(game.rnd, 0xde, 0x8e, 0x22, 255);
	SDL_RenderFillRect(game.rnd, &ground);
	SDL_SetRenderDrawColor(game.rnd, 0, 0, 0, 255);

	/* Temporary test thingy */
	/*
	int w = 30;
	SDL_Rect platform = {
		(game.w >> 1) - w, game.h >> 1, w, w,
	};
	SDL_SetRenderDrawColor(game.rnd, 0xde, 0x8e, 0x22, 255);
	SDL_RenderFillRect(game.rnd, &platform);
	SDL_SetRenderDrawColor(game.rnd, 0, 0, 0, 255);
	*/
}

static void
gravity(void)
{
	for (int i = 0; i <= game.numplayers; i++) {
		if (player[i].y + player[i].h >= game.h - 20 && player[i].dy >= 0) {
			player[i].dy = 0;
			player[i].falling = 0;
			player[i].y = (game.h - 20) - player[i].h;
		}

		player[i].y += player[i].dy;
		player[i].dy += GRAVITY;
	}
}

static void
movePlayers(void)
{
	/* TODO Check if players are colliding with each other */
	for (int i = 0; i <= game.numplayers; i++) {

		if ((player[i].x <= 0 && player[i].dx < 0)
				|| (player[i].x + player[i].w >= game.w && player[i].dx > 0))
			player[i].dx = 0;


		player[i].dx = player[i].dx * 0.97;
		player[i].x += player[i].dx;
	}
}

void
drw(void)
{
	getSurf();
	SDL_RenderClear(game.rnd);

	gravity();
	movePlayers();

	drwGround();
	drwPlayers();

	SDL_RenderPresent(game.rnd);
}

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
#include <limits.h>
#include <math.h>
#include <SDL.h>
#include <SDL_image.h>
#include <stdbool.h>
#include <stdio.h>

#include "alloc.h"
#include "file.h"
#include "fuyunix.h"
#include "keys.h"

/* Constants */
#define GRAVITY 0.0198f
#define JUMP_ACCEL 0.4f
#define SPEED_ACCEL 0.02f
#define SPEED_MAX 2
#define FRICTION 0.88f
#define PLAYER_SIZE 1

#define FRAME_NUM 3 /* FIXME: Temporary to avoid errors */

#define VIRTUAL_WIDTH 16
#define VIRTUAL_HEIGHT 18
#define STAGE_LENGTH 128

const SDL_Rect level[] = {
	{0, VIRTUAL_HEIGHT-1, STAGE_LENGTH, 1},
	{3, VIRTUAL_HEIGHT-5, 10, 2},
	{(STAGE_LENGTH / 2) - 20, (VIRTUAL_HEIGHT / 2) - 20, 20, 20},
	{9, VIRTUAL_HEIGHT-2, 2, 2},
};

/* structs */
struct Player {
	SDL_Texture *frame[FRAME_NUM];
	SDL_Texture **current;

	double x;
	double y;

	double dx;
	double dy;

	double w;
	double h;

	int falling;
};

struct Game {
	SDL_Window *win;
	SDL_Renderer *rnd;

	int h;
	int w;

	/* Old width / height */
	int ow;
	int oh;

	int cam;

	double scale;
	int numplayers;
	int level;
};

/* Global variables */
static struct Game game;
static struct Player *player;

/* Function declarations */
static void freePlayerTextures(void);
static void loadPlayerTextures(void);


/* Function definitions */
static double
getScale(void)
{
	double h = game.h / VIRTUAL_HEIGHT;
	double w = game.w / VIRTUAL_WIDTH;

	return w < h ? w : h;
}

static int
getX(double x)
{
	if (x == -1)
		return VIRTUAL_WIDTH;
	x -= game.cam;

	return x * game.scale;
}

static int
getW(double x)
{
	if (x == -1)
		return VIRTUAL_WIDTH;

	return x * game.scale;
}

static int
getY(double y)
{
	if (y == -1)
		return VIRTUAL_HEIGHT;

	return y * game.scale;
}

static int
getNewWinSize(void)
{
	SDL_GetWindowSize(game.win, &game.w, &game.h);
	if (game.w != game.ow || game.h != game.oh) {
		game.ow = game.w;
		game.oh = game.h;
		game.scale = getScale();
		return 1;
	}
	return 0;
}

static void
initVariables()
{
	game.level = readSaveFile();

	game.numplayers = 0;

	game.ow = 0;
	game.oh = 0;

	game.cam = 0;

	player = NULL;
}

void
init(int winFlags)
{
	if (SDL_Init(SDL_INIT_VIDEO) < 0) {
		fprintf(stderr, "Unable to initialize SDL: %s\n", SDL_GetError());
		exit(-1);
	}

	game.win = SDL_CreateWindow(NAME, SDL_WINDOWPOS_UNDEFINED,
			SDL_WINDOWPOS_UNDEFINED, 640, 480, winFlags);

	if (game.win == NULL) {
		fprintf(stderr, "Unable to create SDL window: %s\n", SDL_GetError());
		exit(-1);
	}

	game.rnd = SDL_CreateRenderer(game.win, -1,
			SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);

	if (game.rnd == NULL) {
		fprintf(stderr, "Unable to create SDL renderer: %s\n", SDL_GetError());
		exit(-1);
	}

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
drwMenuText(SDL_Surface *s, char *text, int x, int y, double size)
{
	cairo_surface_t *cSurface = cairo_image_surface_create_for_data(
			(unsigned char *)s->pixels, CAIRO_FORMAT_RGB24,
			s->w, s->h, s->pitch);

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

static SDL_Texture *
drwHomeMenu(SDL_Surface *s, int gaps, int focus, int size, int width, int height)
{
	SDL_FillRect(s, NULL, 0);
	SDL_Rect options[3] = {
		{gaps, gaps + size, width, height},
		{gaps, gaps + size * 2, width, height},
		{gaps, gaps + size * 3, width, height},
	};

	if (SDL_FillRects(s, options, 3,
				SDL_MapRGB(s->format, 20, 150, 180)) < 0)
		fprintf(stderr, "%s\n", SDL_GetError());

	if (SDL_FillRect(s, &options[focus],
				SDL_MapRGB(s->format, 20, 190, 180)) < 0)
		fprintf(stderr, "%s\n", SDL_GetError());

	char nplayer[] = {game.numplayers + '1', '\0'};

	double textSize = 32.0;
	double offset = height / 2;
	drwMenuText(s, NAME, gaps*2, offset, textSize*2);

	offset += gaps;
	drwMenuText(s, "Start", gaps * 2, offset + size, textSize);
	drwMenuText(s, "Choose players", gaps * 2, offset + size * 2, textSize);
	drwMenuText(s, nplayer, width - gaps * 2, offset + size * 2, textSize);
	drwMenuText(s, "Exit", gaps*2, offset + size * 3, textSize);

	return SDL_CreateTextureFromSurface(game.rnd, s);;
}

void
handleHomeMenu(void)
{
	bool notSelected = true;
	bool redraw = false;
	int focus = 0;
	int prevFocus = 0;
	int gaps = 0;
	int sectionSize = 0;
	int sectionWidth = 0;
	int sectionHeight = 0;
	SDL_Surface *s = SDL_CreateRGBSurface(0, game.w, game.h, 32, 0, 0, 0, 255);
	SDL_Texture *t = drwHomeMenu(s, gaps, focus, sectionSize, sectionWidth, sectionHeight);

	while (1) {
		while (notSelected) {
			/* 2 is the number of options for the player to choose */
			notSelected = handleMenuKeys(&focus, 2);

			if (getNewWinSize()) {
				sectionSize = game.h / 4;
				SDL_FreeSurface(s);
				s = SDL_CreateRGBSurface(0, game.w, game.h, 32, 0, 0, 0, 0);
				if (s == NULL) {
					fprintf(stderr, "SDL_CreateRGBSurface: %s\n", SDL_GetError());
					exit(1);
				}
				/* Arbitary number to get small gaps between selections */
				gaps = game.h / 100;
				sectionHeight = sectionSize - gaps * 2;
				sectionWidth = game.w - gaps * 2;

				redraw = true;
			} else if (prevFocus != focus) {
				redraw = true;
				prevFocus = focus;
			}
			if (redraw) {
				SDL_DestroyTexture(t);
				t = drwHomeMenu(s, gaps, focus, sectionSize, sectionWidth, sectionHeight);
				if (t == NULL) {
					fprintf(stderr, "SDL_CreateTextureFromSurface: %s\n", SDL_GetError());
				}
				redraw = false;
			}
			SDL_RenderCopy(game.rnd, t, NULL, NULL);
			SDL_RenderPresent(game.rnd);
		}
		if (focus == 1) {
			if (game.numplayers >= 1)
				game.numplayers = 0;
			else
				game.numplayers++;

			notSelected = true;
			redraw = true;
		} else if (focus == 2) {
			quitloop();
			return;
		} else {
			break;
		}
	}
	SDL_FreeSurface(s);
	SDL_DestroyTexture(t);
	loadPlayerTextures();
}

void
drwMenu(int player)
{
	/* TODO Draw a menu */
	quitloop();
}

static void
formatPath(char *path, char *resPath, int i, int frame)
{
	if (sprintf(path, "%s%s%d%s%d%s", resPath, "/fuyunix/data/", i,
				"/sprite-", frame, ".png") < 0) {
		perror("sprintf: ");
	};
}

static void
loadPlayerImages(int i)
{
	char *userDir = getenv("XDG_DATA_HOME");
	for (int frame = 0; frame < FRAME_NUM; frame++) {
		char path[PATH_MAX];
		formatPath(path, userDir, i, frame);

		player[i].frame[frame] = IMG_LoadTexture(game.rnd, path);

		if (player[i].frame[frame] == NULL) {
			formatPath(path, RESOURCE_PATH, i, frame);
			player[i].frame[frame] = IMG_LoadTexture(game.rnd, path);

			if (player[i].frame[frame] == NULL) {
				fprintf(stderr, "Unable to Load Texture: %s\n",
						IMG_GetError());
			}
		}
	}
}

static void
loadPlayerTextures(void)
{
	if (IMG_Init(IMG_INIT_PNG) != IMG_INIT_PNG) {
		fprintf(stderr, "IMG_Init: %s\n", IMG_GetError());
	}
	player = (struct Player *)qcalloc(
			game.numplayers + 1, sizeof(struct Player));

	for (int i = 0; i <= game.numplayers; i++) {
		loadPlayerImages(i);

		player[i].current = &player[i].frame[0];

		player[i].x  = 0;
		player[i].y  = 0;
		player[i].dx = 0;
		player[i].dy = 0;

		player[i].w = PLAYER_SIZE;
		player[i].h = PLAYER_SIZE;
	}
	IMG_Quit();
}

static void
freePlayerTextures(void)
{
	if (player == NULL)
		return;

	for (int i = 0; i <= game.numplayers; i++)
		for (int frame = 0; frame < FRAME_NUM; frame++)
			SDL_DestroyTexture(player[i].frame[frame]);

	free(player);
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

	if (player[i].falling)
		return;

	player[i].dy = -JUMP_ACCEL;

	player[i].falling = 1;
}

void
right(int i)
{
	i = i > game.numplayers ? 0 : i;

	if (player[i].x + player[i].w >= STAGE_LENGTH)
		return;

	if (player[i].dx < SPEED_MAX)
		player[i].dx += SPEED_ACCEL;

	player[i].current = &player[i].frame[1];
}

void
left(int i)
{
	i = i > game.numplayers ? 0 : i;

	if (player[i].x <= 0)
		return;

	if (player[i].dx > -SPEED_MAX)
		player[i].dx -= SPEED_ACCEL;

	player[i].current = &player[i].frame[2];
}

static double
gravityCollision(int i)
{
	double dy = player[i].dy;

	/* Player isn't moving and collision detection is unnecessary */
	if (dy == 0)
		return player[i].y;

	double (*roundFunc)(double);
	if (dy < 0)
		roundFunc = floor;
	else
		roundFunc = ceil;

	SDL_Rect p = {
		(int)player[i].x,
		(int)(player[i].y + roundFunc(dy)),
		(int)player[i].w,
		(int)player[i].h
	};
	for (int j = 0; j < (int)(sizeof(level) / sizeof(level[0])); j++) {
		if (SDL_HasIntersection(&p, &level[j])) {
			if (dy > 0) { /* Player is going down */
				player[i].dy = 0;
				player[i].falling = 0;
				return level[j].y - player[i].h;
			} else if (dy < 0) { /* Player is going up */
				player[i].dy = 0;
				return level[j].y + level[j].h;
			}
		}
	}
	return player[i].y + dy;
}

static void
gravity(int i)
{
	if (player[i].dy > 5) {
		player[i].dy = 5;
	} else if (player[i].dy < -5) {
		player[i].dy = -5;
	}

	player[i].y = gravityCollision(i);
	player[i].dy += GRAVITY;
}

static double
collisionDetection(int i)
{
	double dx = player[i].dx;

	/* Player isn't moving and collision detection is unnecessary */
	if (dx == 0)
		return player[i].x;

	double (*roundFunc)(double);
	if (dx < 0)
		roundFunc = floor;
	else
		roundFunc = ceil;

	SDL_Rect p = {
		(int)(player[i].x + roundFunc(dx)),
		(int)player[i].y,
		(int)player[i].w,
		(int)player[i].h
	};
	for (int j = 0; j < (int)(sizeof(level) / sizeof(level[0])); j++) {
		if (SDL_HasIntersection(&p, &level[j])) {
			if (dx > 0) { /* Player is going right */
				player[i].dx = 0;
				return level[j].x - player[i].w;
			} else if (dx < 0) { /* Player is going left */
				player[i].dx = 0;
				return level[j].x + level[j].w;
			}
		}
	}
	return player[i].x + dx;
}

static void
movePlayers(void)
{
	/* TODO Player-Player collision */
	for (int i = 0; i <= game.numplayers; i++) {
		gravity(i);

		if ((player[i].x <= 0 && player[i].dx < 0)
				|| (player[i].x + player[i].w >= STAGE_LENGTH
					&& player[i].dx > 0)) {
			player[i].dx = 0;
		}

		/* TODO: fix the camera for 2 player mode and make scrolling smoother */
		if (player[i].x - game.cam > VIRTUAL_WIDTH * 0.8) {
			game.cam++;
			continue;
		} else if (player[i].x - game.cam <= VIRTUAL_WIDTH * 0.2 && game.cam != 0) {
			game.cam--;
			continue;
		}

		if (player[i].dx >= -0.05 && player[i].dx <= 0.05) {
			player[i].current = &player[i].frame[0];
		}

		player[i].dx = player[i].dx * FRICTION;
		player[i].x = collisionDetection(i);
	}
}

static void
drwPlayers(void)
{
	for (int i = 0; i <= game.numplayers; i++) {
		SDL_Rect playrect = {
			getX(player[i].x), getY(player[i].y),
			getW(player[i].w), getY(player[i].h)
		};

		/* Draw a black square in place of texture that failed to load */
		if (*player[i].current == NULL) {
			SDL_SetRenderDrawColor(game.rnd, 0, 0, 0, SDL_ALPHA_OPAQUE);
			SDL_RenderFillRect(game.rnd, &playrect);
			SDL_SetRenderDrawColor(game.rnd, 0, 0, 0, SDL_ALPHA_OPAQUE);
		} else {
			if (SDL_RenderCopy(game.rnd, *player[i].current, NULL,
						&playrect) < 0) {
				fprintf(stderr, "%s\n", SDL_GetError());
			}
		}
	}
}

static void
drwPlatforms(void)
{
	SDL_SetRenderDrawColor(game.rnd, 0xed, 0xed, 0xed, SDL_ALPHA_OPAQUE);
	for (int i = 0; i < (int)(sizeof(level) / sizeof(level[0])); i++) {
		SDL_Rect l;
		l.x = getX((double)level[i].x);
		l.y = getY((double)level[i].y);
		l.w = getW((double)level[i].w);
		l.h = getY((double)level[i].h);

		SDL_RenderFillRect(game.rnd, &l);
	}

	SDL_SetRenderDrawColor(game.rnd, 0, 0, 0, SDL_ALPHA_OPAQUE);
}

void
drw(void)
{
	getNewWinSize();

	SDL_SetRenderDrawColor(game.rnd, 0, 0, 0, SDL_ALPHA_OPAQUE);
	SDL_RenderClear(game.rnd);

	/* Change background to color: "#114261" */
	SDL_SetRenderDrawColor(game.rnd, 0x11, 0x41, 0x61, SDL_ALPHA_OPAQUE);
	SDL_Rect r = {0, 0, getX(STAGE_LENGTH), getY(VIRTUAL_HEIGHT)};
	SDL_RenderFillRect(game.rnd, &r);

	movePlayers();

	drwPlatforms();
	drwPlayers();

	SDL_RenderPresent(game.rnd);
}

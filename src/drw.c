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

/* Constants */
#define GRAVITY 0.055f
#define JUMP_ACCEL 2
#define SPEED_MAX 6
#define SPEED_ACCEL 0.2f
#define FRICTION 0.88f
#define CAMERA_WIDTH 100
#define CAMERA_HEIGHT 100
#define PLAYER_SIZE 5
#define FRAME_NUM 4

SDL_Rect level[] = {
	/* Ground */
	{0, -1, 100, -2},
	{0, 0, 1, -1},

	/* Arbitary level coordinates / sizes */
	{0, 8, 10, 8},
	{3, 2, 10, 3},
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

	SDL_Surface *surf;

	int h;
	int w;

	/* Old width / height */
	int ow;
	int oh;

	int numplayers;
	int level;
};

struct Camera {
	int x;
	int y;
	int w;
	int h;
	int max_x;
	int max_y;
};

/* Global variables */
static struct Game game;
static struct Player *player;
static struct Camera camera;

/* Function declarations */
static void freePlayerTextures();
static void loadPlayerTextures();


/* Function definitions */
static int
getX(int x)
{
	if (x == -1)
		return game.w;
	return x * game.w / camera.w;
}

static int
getY(int y)
{
	if (y == -1)
		return game.h;
	return y * game.h / camera.h;
}

static void
initVariables()
{
	game.level = readSaveFile();

	game.numplayers = 0;

	game.ow = 0;
	game.oh = 0;

	camera.x = 0;
	camera.y = 0;
	/* Use a separate coordinate system to track stuff */
	camera.w = CAMERA_WIDTH;
	camera.h = CAMERA_HEIGHT;
	camera.max_x = 1000;

	/* No vertical movement */
	camera.max_y = 100;

	player = NULL;
}

void
init(void)
{
	if (SDL_Init(SDL_INIT_VIDEO) < 0) {
		fprintf(stderr, "Unable to initialize SDL: %s\n", SDL_GetError());
		exit(-1);
	}

	game.win = SDL_CreateWindow(NAME, SDL_WINDOWPOS_UNDEFINED,
			SDL_WINDOWPOS_UNDEFINED, 640, 480, 0);

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
loadPlayerImages(int i)
{
	/* string length for all of them are the same */
	size_t size = strlen(RESOURCE_PATH "/data/0/sprite-0.png") + 1;

	for (int frame = 0; frame < FRAME_NUM; frame++) {
		char *str = qcalloc(size, sizeof(char));
		sprintf(str, "%s%s%d%s%d%s", RESOURCE_PATH, "/data/", i, "/sprite-",
				frame, ".png");

		player[i].frame[frame] = IMG_LoadTexture(game.rnd, str);

		if (player[i].frame[frame] == NULL)
			fprintf(stderr, "Unable to Load Texture: %s\n", IMG_GetError());

		free(str);
	}
}

static void
loadPlayerTextures(void)
{
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
}

static void
freePlayerTextures(void)
{
	/* Causes segfaults if ignored */
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

	if (player[i].y <= 0 || player[i].falling)
		return;

	player[i].dy = -JUMP_ACCEL;

	player[i].falling = 1;
}

void
right(int i)
{
	i = i > game.numplayers ? 0 : i;

	if ((player[i].x + player[i].w) >= game.w)
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
gravity(void)
{
	for (int i = 0; i <= game.numplayers; i++) {
		if (player[i].y + player[i].h >= camera.h - 1 && player[i].dy >= 0) {
			player[i].dy = 0;
			player[i].falling = 0;
			player[i].y = (camera.h - 1) - player[i].h;
		}

		if (player[i].dy > 10) {
			player[i].dy = 10;
		} else if (player[i].dy < -10) {
			player[i].dy = -10;
		}

		player[i].y += player[i].dy;
		player[i].dy += GRAVITY;
	}
}

static void
drwPlayers(void)
{
	for (int i = 0; i <= game.numplayers; i++) {
		SDL_Rect playrect = {
			getX(player[i].x), getY(player[i].y),
			getX(player[i].w), getY(player[i].h)
		};

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

static void
movePlayers(void)
{
	/* TODO Check if players are colliding with each other */

	for (int i = 0; i <= game.numplayers; i++) {
		if ((player[i].x <= 0 && player[i].dx < 0)
				|| (player[i].x + player[i].w >= camera.w && player[i].dx > camera.x))
			player[i].dx = 0;

		if (player[i].x > camera.w * 0.8) {
			camera.x++;
			player[i].x--;
		}

		if (player[i].dx >= -0.06 && player[i].dx <= 0.06) {
			player[i].current = &player[i].frame[0];
		}

		player[i].dx = player[i].dx * FRICTION;
		player[i].x += player[i].dx;
	}
}

static void
drwPlatforms(void)
{
	SDL_SetRenderDrawColor(game.rnd, 0xde, 0x8e, 0x22, SDL_ALPHA_OPAQUE);
	for (int i = 0; i < (int)(sizeof(level) / sizeof(level[0])); i++) {
		SDL_Rect l;
		l.x = getX(level[i].x);
		l.y = getY(level[i].y);
		l.w = getX(level[i].w);
		l.h = getY(level[i].h);

		SDL_RenderFillRect(game.rnd, &l);
	}

	SDL_SetRenderDrawColor(game.rnd, 0, 0, 0, 255);
}

void
drw(void)
{
	getSurf();

	/* Change background to color: "#114394" */
	SDL_SetRenderDrawColor(game.rnd, 0x11, 0x43, 0x94, SDL_ALPHA_OPAQUE);

	SDL_RenderClear(game.rnd);

	/*
	 * TODO platform-player and player-player collision detection.
	 * Move players in small steps and check collision everytime.
	 * Use custom positioning (Maxed at 20 for y axis and 200 for x axis).
	 * One screen can display 50 "blocks" on x and 20 "blocks" on y
	 * Create helper functions to map custom positions to screen position.
	 * TODO Objects line up independent of screen size
	 */
	gravity();
	movePlayers();

	drwPlatforms();
	drwPlayers();

	SDL_RenderPresent(game.rnd);
}

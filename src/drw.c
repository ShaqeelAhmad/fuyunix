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

#include <assert.h>
#include <limits.h>
#include <math.h>
#include <SDL.h>
#include <SDL_image.h>
#include <SDL_ttf.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "util.h"
#include "fuyunix.h"
#include "keys.h"
#include "scfg.h"

#define GRAVITY 3.48f
#define TERMINAL_VELOCITY (GRAVITY * 2.8)
#define JUMP_ACCEL 8
#define SPEED_ACCEL 2.02f
#define SPEED_MAX 8
#define FRICTION 0.78f

#define FRAME_NUM 3

#define BLOCK_SIZE 32
#define PLAYER_SIZE BLOCK_SIZE
#define LOGICAL_WIDTH 800
#define LOGICAL_HEIGHT 450
#define MAX_STAGE_HEIGHT (LOGICAL_HEIGHT * 4)
#define MAX_STAGE_LENGTH (LOGICAL_WIDTH * 10)

size_t stage_height = MAX_STAGE_HEIGHT;
size_t stage_length = MAX_STAGE_LENGTH;

struct Region {
	SDL_Texture *t;
	SDL_Rect rect;
};

struct Level {
	size_t stage_length;
	struct Region *regions;
	size_t regions_len;
};

struct Level level;

enum GameState {
	STATE_MENU,
	STATE_PLAY,
	STATE_PAUSE,
	STATE_DEAD,
	STATE_WON,
};

struct Player {
	SDL_Texture *frame[FRAME_NUM];
	SDL_Texture **current;

	double x;
	double y;

	double dx;
	double dy;

	double w;
	double h;

	bool inAir;
};

struct Game {
	TTF_Font *font;
	SDL_Window *win;
	SDL_Renderer *rnd;

	enum GameState state;

	bool focusSelect;
	int focusChange;
	int menuFocus;

	struct timespec time[2];
	int curtime;
	double dt;

	int h;
	int w;

	double fps;
	double cam_x;
	double cam_y;

	double scale;
	int numplayers;
	int level;
};

static struct Game game;
static struct Player player[2];

struct TileTexture {
	char *name;
	SDL_Texture *tile;
};

static struct TileTexture tileTextures[] = {
	{.name = "snow"},
	{.name = "end"},
};
static SDL_Texture *endPointTexture = NULL;

void
initTileTextures(void)
{
	char *tileDir = GAME_DATA_DIR"/tiles/";
	char *ext = ".png";
	char tileFile[PATH_MAX];
	for (size_t i = 0; i < (sizeof(tileTextures) / sizeof(tileTextures[0])); i++) {
		struct TileTexture *t = &tileTextures[i];
		strcpy(tileFile, tileDir);
		strcat(tileFile, t->name);
		strcat(tileFile, ext);
		t->tile = IMG_LoadTexture(game.rnd, tileFile);
		if (t->tile == NULL) {
			fprintf(stderr, "can't load tile %s from file %s", t->name, tileFile);
			continue;
		}
		if (strcmp(t->name, "end") == 0) {
			endPointTexture = t->tile;
		}
	}
}

SDL_Texture *
getTileTexture(char *s)
{
	for (size_t i = 0; i < sizeof(tileTextures) / sizeof(tileTextures[0]); i++) {
		struct TileTexture *t = &tileTextures[i];
		if (strcmp(t->name, s) == 0) {
			return t->tile;
		}
	}
	return NULL;
}

static void
negativeDie(int x)
{
	if (x < 0) {
		fprintf(stderr, "%s\n", SDL_GetError());
		exit(-1);
	}
}

static void *
nullDie(void *p)
{
	if (p == NULL) {
		fprintf(stderr, "%s\n", SDL_GetError());
		exit(-1);
	}
	return p;
}

static void
formatPath(char *path, char *resPath, int i, int frame)
{
	if (sprintf(path, "%s/%d%s%d%s", resPath, i,
				"/sprite-", frame, ".png") < 0) {
		perror("sprintf: ");
	};
}

static void
loadPlayerImages(int i)
{
	char *userDir = getenv("XDG_DATA_HOME");
	char path[PATH_MAX];
	for (int frame = 0; frame < FRAME_NUM; frame++) {
		formatPath(path, userDir, i, frame);

		player[i].frame[frame] = IMG_LoadTexture(game.rnd, path);

		if (player[i].frame[frame] == NULL) {
			formatPath(path, GAME_DATA_DIR, i, frame);
			player[i].frame[frame] = IMG_LoadTexture(game.rnd, path);

			if (player[i].frame[frame] == NULL) {
				fprintf(stderr, "Unable to load image texture: %s\n",
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
		exit(1);
	}

	game.cam_x = 0;
	game.cam_y = 0;
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
	for (int i = 0; i <= game.numplayers; i++)
		for (int frame = 0; frame < FRAME_NUM; frame++)
			SDL_DestroyTexture(player[i].frame[frame]);
}

static void
loadLevels(void)
{
	struct scfg_block block;

	char *levelPath = GAME_DATA_DIR"/levels/1";

	// TODO: Allow loading levels from $XDG_DATA_HOME
	if (scfg_load_file(&block, levelPath) < 0) {
		perror(levelPath);
		exit(1);
	}

	level.regions = ecalloc(block.directives_len, sizeof(struct Region));
	level.regions_len = 0;
	level.stage_length = MAX_STAGE_LENGTH;
	for (size_t i = 0; i < block.directives_len; ++i) {
		if (strcmp(block.directives[i].name, "stage_length") == 0) {
			if (block.directives[i].params_len != 1) {
				fprintf(stderr, "%s:%d Expected 1 field for stage_length got %zu\n",
						levelPath,
						block.directives[i].lineno,
						block.directives[i].params_len);
			}
			level.stage_length = atoi(block.directives[i].params[0]);
			if (level.stage_length <= 0 || level.stage_length > MAX_STAGE_LENGTH)
				level.stage_length = MAX_STAGE_LENGTH;
			else
				level.stage_length *= BLOCK_SIZE;
			continue;
		}

		if (block.directives[i].params_len != 4) {
			fprintf(stderr, "%s:%d Expected 5 fields got %zu\n", levelPath,
					block.directives[i].lineno,
					block.directives[i].params_len);
			continue;
		};

		SDL_Texture *t = getTileTexture(block.directives[i].name);
		if (t == NULL) {
			fprintf(stderr, "%s:%d: unknown tile %s\n", levelPath,
					block.directives[i].lineno,
					block.directives[i].name);
			continue;
		}

		int coords[4] = {0};
		coords[0] = atoi(block.directives[i].params[0]) * BLOCK_SIZE;
		coords[1] = atoi(block.directives[i].params[1]) * BLOCK_SIZE;
		coords[2] = atoi(block.directives[i].params[2]) * BLOCK_SIZE;
		coords[3] = atoi(block.directives[i].params[3]) * BLOCK_SIZE;

		level.regions[level.regions_len] = (struct Region){
			.t = t,
			.rect = (SDL_Rect){
				coords[0],
				coords[1],
				coords[2],
				coords[3],
			},
		};
		level.regions_len++;
	}
	stage_length = level.stage_length;
	if (level.regions_len == 0) {
		fprintf(stderr, "Unable to read any data from file %s\n", levelPath);
		exit(1);
	}
	scfg_block_finish(&block);
}

static void
initVariables(void)
{
	game.state = STATE_MENU;
	game.level = readSaveFile();

	game.numplayers = 0;

	game.w = LOGICAL_WIDTH;
	game.h = LOGICAL_HEIGHT;

	game.cam_x = 0;
	game.cam_y = 0;

	game.curtime = 0;
	clock_gettime(CLOCK_MONOTONIC, &game.time[0]);
	game.time[1] = game.time[0];

	game.fps = 1.0 / 60.0;

	int i = SDL_GetWindowDisplayIndex(game.win);
	if (i < 0) {
		fprintf(stderr, "Unable to get display index: %s\n", SDL_GetError());
		return;
	}

	SDL_DisplayMode dm;
	if (SDL_GetCurrentDisplayMode(i, &dm) < 0) {
		fprintf(stderr, "Unable to get display mode: %s\n", SDL_GetError());
		return;
	}
	game.fps = 1.0 / (double)dm.refresh_rate;

	initTileTextures();
	loadLevels();
}

void
initFont(void)
{
	if (TTF_Init() < 0) {
		fprintf(stderr, "SDL_TTF: TTF_Init: %s\n", TTF_GetError());
		exit(1);
	}

	char *file = GAME_DATA_DIR"/fonts/FreeSerifBoldItalic.ttf";
	game.font = TTF_OpenFont(file, 32);
	if (game.font == NULL) {
		fprintf(stderr, "SDL_TTF: TTF_OpenFont for file %s: %s\n",
				file, TTF_GetError());
		exit(1);
	}
}

void
init(int winFlags)
{
	negativeDie(SDL_Init(SDL_INIT_VIDEO));

	game.win = nullDie(SDL_CreateWindow(NAME, SDL_WINDOWPOS_UNDEFINED,
				SDL_WINDOWPOS_UNDEFINED, LOGICAL_WIDTH, LOGICAL_HEIGHT,
				winFlags));

	game.rnd = nullDie(SDL_CreateRenderer(game.win, -1,
				SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC));

	SDL_RenderSetLogicalSize(game.rnd, LOGICAL_WIDTH, LOGICAL_HEIGHT);

	loadConfig();

	initVariables();

	initFont();
}

void
cleanup(void)
{
	freeKeys();

	freePlayerTextures();

	free(level.regions);
	level.regions_len = 0;

	writeSaveFile(game.level);

	TTF_CloseFont(game.font);
	TTF_Quit();
	SDL_DestroyRenderer(game.rnd);
	SDL_DestroyWindow(game.win);

	SDL_Quit();
}

static void
drwText(char *text, int x, int y, int size)
{
	TTF_SetFontSize(game.font, size);

	SDL_Color fg = {
		.a = 0xFF,
		.r = 0xFF,
		.g = 0xFF,
		.b = 0xFF,
	};
	SDL_Surface *s = TTF_RenderUTF8_Solid(game.font, text, fg);
	if (s == NULL) {
		fprintf(stderr, "SDL_TTF: TTF_RenderUTF8_Solid: %s\n", TTF_GetError());
		exit(1);
	}
	SDL_Texture *t = nullDie(SDL_CreateTextureFromSurface(game.rnd, s));

	SDL_Rect dest = {
		.x = x,
		.y = y,
		.w = s->w,
		.h = s->h,
	};
	SDL_RenderCopy(game.rnd, t, NULL, &dest);

	SDL_DestroyTexture(t);
	SDL_FreeSurface(s);
}

static void
drwTextScreenCentered(char *text, int size)
{
	TTF_SetFontSize(game.font, size);

	int w = 0, h = 0;
	if (TTF_SizeUTF8(game.font, text, &w, &h) < 0) {
		fprintf(stderr, "SDL_TTF: TTF_SizeUTF8: %s\n", TTF_GetError());
		exit(1);
	}
	int x = game.w / 2 - w / 2;
	int y = game.h / 2 - h / 2;

	drwText(text, x, y, size);
}

static void
drwHomeMenu(int gaps, int focus, int size, int width, int height)
{
	SDL_SetRenderDrawColor(game.rnd, 0, 0, 0, SDL_ALPHA_OPAQUE);
	SDL_RenderClear(game.rnd);

	SDL_Rect options[3] = {
		{gaps, gaps + size, width, height},
		{gaps, gaps + size * 2, width, height},
		{gaps, gaps + size * 3, width, height},
	};

	SDL_SetRenderDrawColor(game.rnd, 20, 150, 180, SDL_ALPHA_OPAQUE);
	if (SDL_RenderFillRects(game.rnd, options, 3) < 0)
		fprintf(stderr, "%s\n", SDL_GetError());

	SDL_SetRenderDrawColor(game.rnd, 20, 190, 180, SDL_ALPHA_OPAQUE);
	if (SDL_RenderFillRect(game.rnd, &options[focus]) < 0)
		fprintf(stderr, "%s\n", SDL_GetError());

	char nplayer[] = {game.numplayers + '1', '\0'};

	int textSize = 32;
	double offset = (double)height / 2.0;
	drwText(NAME, gaps, offset, textSize*2);

	offset += gaps;

	drwText("Start", gaps*2, offset + size, textSize);

	int w = 0;
	if (TTF_SizeUTF8(game.font, nplayer, &w, NULL) < 0) {
		fprintf(stderr, "SDL_TTF: TTF_SizeUTF8: %s\n", TTF_GetError());
		exit(1);
	}

	drwText("Choose players", gaps*2, offset + size * 2, textSize);
	drwText(nplayer, width - w, offset + size * 2, textSize);

	drwText("Exit", gaps*2, offset + size * 3, textSize);
}

void
down(int i)
{
	/* Do nothing for now */
}

void
jump(int i, bool keyReleased)
{
	static bool jumpHigher = false;

#define MIN_PLAYER_DY (-JUMP_ACCEL * 2)
	if (keyReleased || player[i].dy < MIN_PLAYER_DY) {
		jumpHigher = false;
		return;
	}
	if (!jumpHigher && !player[i].inAir) {
		jumpHigher = true;
	}

	if (jumpHigher) {
		i = i > game.numplayers ? 0 : i;

		player[i].dy -= JUMP_ACCEL;

		player[i].inAir = true;
	}
}

void
right(int i)
{
	i = i > game.numplayers ? 0 : i;

	if (player[i].x + player[i].w >= stage_length)
		return;

	if (player[i].dx < SPEED_MAX)
		player[i].dx += SPEED_ACCEL;
	else
		player[i].dx = SPEED_MAX;

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

	SDL_FRect p = {
		player[i].x,
		player[i].y + dy,
		player[i].w,
		player[i].h
	};

	for (size_t j = 0; j < level.regions_len; j++) {
		SDL_FRect r = {
			.x = (float)level.regions[j].rect.x,
			.y = (float)level.regions[j].rect.y,
			.w = (float)level.regions[j].rect.w,
			.h = (float)level.regions[j].rect.h,
		};
		if (SDL_HasIntersectionF(&p, &r)) {
			if (level.regions[j].t == endPointTexture) {
				game.state = STATE_WON;
				return player[i].y;
			}
			player[i].dy = 0;
			if (dy > 0) { /* Player hit a platform while falling */
				player[i].inAir = false;
				return r.y - player[i].h;
			} else if (dy < 0) { /* Player bonked while jumping */
				return r.y + r.h;
			}
		}
	}
	if (dy > 0) {
		player[i].inAir = true;
	}

	return player[i].y + dy;
}

static void
gravity(int i, double dt)
{
	if (player[i].dy > TERMINAL_VELOCITY) {
		player[i].dy = TERMINAL_VELOCITY;
	}

	player[i].y = gravityCollision(i);
	player[i].dy += GRAVITY;

	if (player[i].y > stage_height) {
		game.state = STATE_DEAD;
	}
}

static double
collisionDetection(int i)
{
	double dx = player[i].dx;

	/* Player isn't moving and collision detection is unnecessary */
	if (dx == 0)
		return player[i].x;

	SDL_FRect p = {
		player[i].x + dx,
		player[i].y,
		player[i].w,
		player[i].h
	};

	for (size_t j = 0; j < level.regions_len; j++) {
		SDL_FRect r = {
			.x = (float)level.regions[j].rect.x,
			.y = (float)level.regions[j].rect.y,
			.w = (float)level.regions[j].rect.w,
			.h = (float)level.regions[j].rect.h,
		};
		if (SDL_HasIntersectionF(&p, &r)) {
			if (level.regions[j].t == endPointTexture) {
				game.state = STATE_WON;
				return player[i].x;
			}

			if (dx > 0) { /* Player is going right */
				player[i].dx = 0;
				return r.x - player[i].w;
			} else if (dx < 0) { /* Player is going left */
				player[i].dx = 0;
				return r.x + r.w;
			}
		}
	}

	return player[i].x + dx;
}

static void
movePlayers(int64_t dt)
{
	double total_x = 0;
	double total_y = 0;

	/* TODO: Player-Player collision */
	for (int i = 0; i <= game.numplayers; i++) {
		gravity(i, dt);

		if ((player[i].x <= 0 && player[i].dx < 0)
				|| (player[i].x + player[i].w >= stage_length
					&& player[i].dx > 0)) {
			player[i].dx = 0;
		}

		if (player[i].dx >= -0.05 && player[i].dx <= 0.05) {
			player[i].current = &player[i].frame[0];
		}

		player[i].dx = player[i].dx * FRICTION;// * dt;
		player[i].x = collisionDetection(i);


		/* TODO: 2 player mode. Don't allow players to go too far offscreen. */
		if (player[i].x + player[i].w >= stage_length || player[i].x <= 0) {
			player[i].dx = 0;
		}

		total_x += player[i].x;
		total_y += player[i].y;
	}

	double avg_x = total_x / (double)(game.numplayers + 1);

	if (avg_x > LOGICAL_WIDTH / 2 && avg_x < stage_length - LOGICAL_WIDTH / 2) {
		if (avg_x - game.cam_x < -0.9 || avg_x - game.cam_x > 0.9) {
			game.cam_x = avg_x - (LOGICAL_WIDTH / 2);
		}
	}

	double avg_y = total_y / (double)(game.numplayers + 1);
	game.cam_y = avg_y - LOGICAL_HEIGHT / 2;
	if (game.cam_y > stage_height - LOGICAL_WIDTH) {
		game.cam_y = stage_height - LOGICAL_WIDTH;
	}
}

static void
drwPlayers(void)
{
	for (int i = 0; i <= game.numplayers; i++) {
		SDL_Rect playrect = {
			player[i].x - game.cam_x,
			player[i].y - game.cam_y,
			player[i].w, player[i].h,
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
	for (size_t i = 0; i < level.regions_len; i++) {
		SDL_Rect r = {
			.x = level.regions[i].rect.x - game.cam_x,
			.y = level.regions[i].rect.y - game.cam_y,
			.w = level.regions[i].rect.w,
			.h = level.regions[i].rect.h,
		};

		if (r.x > LOGICAL_WIDTH) {
			continue;
		}

		for (int y = r.y; y < r.y + r.h; y += BLOCK_SIZE) {
			SDL_Rect f = r;
			f.y = y;
			f.h = BLOCK_SIZE;

			if (y + BLOCK_SIZE < 0 || y > LOGICAL_HEIGHT) {
				continue;
			}
			for (int x = r.x; x < r.x + r.w; x += BLOCK_SIZE) {
				if (x + BLOCK_SIZE < 0 || x > LOGICAL_WIDTH) {
					continue;
				}
				f.x = x;
				f.w = BLOCK_SIZE;

				SDL_RenderCopy(game.rnd, level.regions[i].t, NULL, &f);
			}
		}
	}
}

void
drw(void)
{
	SDL_Rect screenRect = {0, 0, game.w, game.h};

	static int death_alpha = 0;
	if (game.state != STATE_DEAD) death_alpha = 0;

	switch (game.state) {
	case STATE_MENU:
		SDL_SetRenderDrawColor(game.rnd, 0, 0, 0, SDL_ALPHA_OPAQUE);
		SDL_RenderClear(game.rnd);

		int gaps = game.h / 100;
		int sectionSize = game.h / 4;
		int sectionHeight = sectionSize - gaps * 2;
		int sectionWidth = game.w - gaps * 2;
		game.menuFocus += game.focusChange;
		if (game.menuFocus < 0) game.menuFocus = 0;
		if (game.menuFocus > 2) game.menuFocus = 2;
		game.focusChange = 0;

		if (game.focusSelect) {
			switch (game.menuFocus) {
			case 0:
				game.state = STATE_PLAY;
				loadPlayerTextures();
				break;
			case 1:
				if (game.numplayers >= 1)
					game.numplayers = 0;
				else
					game.numplayers++;
				break;
			case 2:
				quitloop();
				break;
			}
		}
		game.focusSelect = 0;

		drwHomeMenu(gaps, game.menuFocus,
					sectionSize, sectionWidth, sectionHeight);
		break;
	case STATE_PLAY:
		SDL_SetRenderDrawColor(game.rnd, 0, 0, 0, SDL_ALPHA_OPAQUE);
		SDL_RenderClear(game.rnd);
		/* Change background to color: "#114261" */
		SDL_SetRenderDrawColor(game.rnd, 0x11, 0x41, 0x61, SDL_ALPHA_OPAQUE);
		SDL_RenderFillRect(game.rnd, &screenRect);

		movePlayers(game.dt);

		drwPlatforms();
		drwPlayers();
		break;
	case STATE_WON: {
		// TODO: menu or at least keys to restart the level or go back
		// to main menu
		SDL_SetRenderDrawColor(game.rnd, 0, 0, 0, SDL_ALPHA_OPAQUE);
		SDL_RenderClear(game.rnd);

		drwTextScreenCentered("You won", 40);
		break;
	}
	case STATE_DEAD: {
		// TODO: menu or at least keys to restart the level or go back
		// to main menu
		SDL_SetRenderDrawColor(game.rnd, 0, 0, 0, SDL_ALPHA_OPAQUE);
		SDL_RenderClear(game.rnd);
		if (death_alpha < 255) {
			SDL_SetRenderDrawColor(game.rnd, 0x11, 0x41, 0x61, SDL_ALPHA_OPAQUE);
			SDL_RenderFillRect(game.rnd, &screenRect);
			drwPlatforms();
			drwPlayers();

			SDL_SetRenderDrawBlendMode(game.rnd, SDL_BLENDMODE_BLEND);
			SDL_SetRenderDrawColor(game.rnd, 0, 0, 0, death_alpha);
			SDL_RenderFillRect(game.rnd, NULL);
		}

		drwTextScreenCentered("You died", 40);

		if (death_alpha < 255)
			death_alpha += 2;

		if (death_alpha > 255)
			death_alpha = 255;

		break;
	}
	case STATE_PAUSE:
		SDL_SetRenderDrawColor(game.rnd, 0, 0, 0, SDL_ALPHA_OPAQUE);
		SDL_RenderClear(game.rnd);
		SDL_SetRenderDrawColor(game.rnd, 0x11, 0x41, 0x61, SDL_ALPHA_OPAQUE);
		SDL_RenderFillRect(game.rnd, &screenRect);

		drwPlatforms();
		drwPlayers();

		// TODO: a pause menu
		SDL_Rect r = {
			.x = game.w / 2 - BLOCK_SIZE,
			.y = game.h / 2 - 2 * BLOCK_SIZE,
			.w = BLOCK_SIZE,
			.h = BLOCK_SIZE*4,
		};
		SDL_SetRenderDrawColor(game.rnd, 0x11, 0x11, 0x11, SDL_ALPHA_OPAQUE);
		SDL_RenderFillRect(game.rnd, &r);
		r.x = game.w / 2 + BLOCK_SIZE;
		SDL_RenderFillRect(game.rnd, &r);
		break;
	}

#define SECOND 1e9

	clock_gettime(CLOCK_MONOTONIC, &game.time[game.curtime]);
	struct timespec *t1 = &game.time[game.curtime];
	struct timespec *t2 = &game.time[!game.curtime];
	int64_t dt = ((t1->tv_sec - t2->tv_sec) * SECOND)
		+ t1->tv_nsec - t2->tv_nsec;

	game.curtime = !game.curtime;

	game.dt = (double)dt * 1e-6; /* convert nanosecond to millisecond */

	if (game.dt < game.fps) {
		SDL_Delay(game.fps - game.dt);
	}

	SDL_RenderPresent(game.rnd);

	clock_gettime(CLOCK_MONOTONIC, &game.time[game.curtime]);
	game.curtime = !game.curtime;

}

void
menuHandleKey(int sym)
{
	switch (game.state) {
	case STATE_PLAY:
		if (sym == KEY_PAUSE || sym == KEY_QUIT)
			game.state = STATE_PAUSE;
		break;
	case STATE_MENU:
		switch (sym) {
		case KEY_UP:
			game.focusChange = -1;
			break;
		case KEY_DOWN:
			game.focusChange = 1;
			break;
		case KEY_LEFT:
			game.focusChange = -1;
			break;
		case KEY_RIGHT:
			game.focusChange = 1;
			break;
		case KEY_PAUSE:
			break;
		case KEY_SELECT:
			game.focusSelect = true;
			break;
		case KEY_QUIT:
			quitloop();
			break;
		};
		break;
	case STATE_WON:
		/* FALLTHROUGH */
	case STATE_DEAD:
		/* FALLTHROUGH */
	case STATE_PAUSE:
		switch (sym) {
		case KEY_UP:
			game.focusChange = -1;
			break;
		case KEY_DOWN:
			game.focusChange = 1;
			break;
		case KEY_LEFT:
			game.focusChange = -1;
			break;
		case KEY_RIGHT:
			game.focusChange = 1;
			break;
		case KEY_PAUSE:
			game.state = STATE_PLAY;
			break;
		case KEY_SELECT:
			game.focusSelect = true;
			break;
		case KEY_QUIT:
			game.state = STATE_MENU;
			break;
		};
		break;
	};
}

void
handleKeyup(int sym, int player)
{
	switch (game.state) {
	case STATE_WON:  /* FALLTHROUGH */
	case STATE_DEAD: /* FALLTHROUGH */
	case STATE_MENU: /* FALLTHROUGH */
	case STATE_PAUSE:
		break;
	case STATE_PLAY:
		if (sym == KEY_UP) {
			jump(player, true);
		}
		break;
	}
}

void
handleKey(int sym, int player)
{
	switch (game.state) {
	case STATE_WON:  /* FALLTHROUGH */
	case STATE_DEAD: /* FALLTHROUGH */
	case STATE_MENU: /* FALLTHROUGH */
	case STATE_PAUSE:
		break;
	case STATE_PLAY:
		switch (sym) {
		case KEY_UP:
			jump(player, false);
			break;
		case KEY_DOWN:
			down(player);
			break;
		case KEY_LEFT:
			left(player);
			break;
		case KEY_RIGHT:
			right(player);
			break;
		case KEY_PAUSE:
		case KEY_QUIT:
			break;
		};
		break;
	};
}

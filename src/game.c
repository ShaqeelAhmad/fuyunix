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

#include <limits.h>
#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>

#include "game.h"
#include "util.h"
#include "fuyunix.h"
#include "scfg.h"

static size_t stage_height = MAX_STAGE_HEIGHT;
static size_t stage_length = MAX_STAGE_LENGTH;

static bool split_screen = true;

struct Region {
	game_Texture *t;
	struct game_Rect rect;
};

struct Level {
	size_t stage_length;
	struct Region *regions;
	size_t regions_len;
};

enum GameState {
	STATE_MENU,
	// TODO: level selection state.
	STATE_PLAY,
	STATE_PAUSE,
	STATE_DEAD,
	STATE_WON,
};

struct Player {
	game_Texture *frame[FRAME_NUM];
	game_Texture **current;

	double x;
	double y;

	struct {
		bool active;
		bool retToPlayer;
		double x;
		double y;
		double initial_x;
		double dx;
		double dy;
	} proj;

	double dx;
	double dy;

	double w;
	double h;

	bool inAir;
};

struct Game {
	enum GameState state;

	bool focusSelect;
	int focusChange;
	int menuFocus;

	double dt;

	int h;
	int w;

	struct {
		double x;
		double y;
		double w;
		double h;
		double cam_x;
		double cam_y;
	} screens[2];

	double scale;
	int numplayers;
	int level;


	int curLevel;
	struct Level *levels;
	int levels_len;

	bool running;
};

static struct Game game;
static struct Player player[2];

struct TileTexture {
	char *name;
	int  len;

	game_Texture *tile;
};

static struct TileTexture tileTextures[] = {
	{.name = "snow"},
	{.name = "end"},
	{.name = "enemy"},
};

static game_Texture *endPointTexture = NULL;

void
initTileTextures(void)
{
	char *tileDir = GAME_DATA_DIR"/tiles/";
	int tileDirLen = strlen(tileDir);
	char *ext = ".png";
	int extLen = strlen(ext);
	char file[PATH_MAX];
	for (size_t i = 0; i < (sizeof(tileTextures) / sizeof(tileTextures[0])); i++) {
		struct TileTexture *t = &tileTextures[i];
		int nameLen = strlen(t->name);
		memcpy(file, tileDir, tileDirLen);
		memcpy(file + tileDirLen, t->name, nameLen);
		memcpy(file + tileDirLen + nameLen, ext, extLen);
		*(file + tileDirLen + nameLen + extLen) = 0;
		t->tile = platform_LoadTexture(file);
		if (t->tile == NULL) {
			platform_Log("can't load tile %s from file %s\n", t->name, file);
			continue;
		}
		if (strcmp(t->name, "end") == 0) {
			endPointTexture = t->tile;
		}
	}
}

game_Texture *
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

bool
game_HasIntersectionF(struct game_FRect a, struct game_FRect b)
{
	return (a.x < b.x + b.w && a.x + a.w > b.x) &&
			(a.y < b.y + b.h && a.y + a.h > b.y);
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

		player[i].frame[frame] = platform_LoadTexture(path);

		if (player[i].frame[frame] == NULL) {
			formatPath(path, GAME_DATA_DIR, i, frame);
			player[i].frame[frame] = platform_LoadTexture(path);

			if (player[i].frame[frame] == NULL) {
				platform_Log("Unable to load image texture: %s\n", path);
			}
		}
	}
}

static void
loadPlayerTextures(void)
{
	game.screens[0].cam_x = 0;
	game.screens[0].cam_y = 0;
	game.screens[0].x = 0;
	game.screens[0].y = 0;
	game.screens[0].w = LOGICAL_WIDTH;
	game.screens[0].h = LOGICAL_HEIGHT;
	if (game.numplayers == 1) {
		game.screens[0].w = LOGICAL_WIDTH/2;
	}

	game.screens[1].cam_x = 0;
	game.screens[1].cam_y = 0;
	game.screens[1].x = LOGICAL_WIDTH/2;
	game.screens[1].y = 0;
	game.screens[1].w = LOGICAL_WIDTH/2;
	game.screens[1].h = LOGICAL_HEIGHT;

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
	for (int i = 0; i <= game.numplayers; i++)
		for (int frame = 0; frame < FRAME_NUM; frame++)
			platform_DestroyTexture(player[i].frame[frame]);
}

static struct Level
loadLevel(char *file)
{
	struct Level level = {0};
	struct scfg_block block;
	if (scfg_load_file(&block, file) < 0) {
		platform_Log("Failed to load file %s\n", file);
		return level;
	}

	level.regions = ecalloc(block.directives_len, sizeof(struct Region));
	level.regions_len = 0;
	level.stage_length = MAX_STAGE_LENGTH;
	for (size_t i = 0; i < block.directives_len; ++i) {
		if (strcmp(block.directives[i].name, "stage_length") == 0) {
			if (block.directives[i].params_len != 1) {
				platform_Log("%s:%d Expected 1 field for stage_length got %d\n",
						file,
						block.directives[i].lineno,
						(int)block.directives[i].params_len);
			}
			level.stage_length = atoi(block.directives[i].params[0]);
			if (level.stage_length <= 0 || level.stage_length > MAX_STAGE_LENGTH)
				level.stage_length = MAX_STAGE_LENGTH;
			else
				level.stage_length *= BLOCK_SIZE;
			continue;
		}

		if (block.directives[i].params_len != 4) {
			platform_Log("%s:%d Expected 5 fields got %d\n",
				file,
				block.directives[i].lineno,
				(int)block.directives[i].params_len);
			continue;
		};

		game_Texture *t = getTileTexture(block.directives[i].name);
		if (t == NULL) {
			platform_Log("%s:%d: unknown tile %s\n", file,
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
				.rect = (struct game_Rect){
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
		platform_Log("Unable to read any data from file %s\n", file);
		free(level.regions);
	}
	scfg_block_finish(&block);

	return level;
}

static void
loadLevels(void)
{
	struct Level *levels = NULL;
	int levels_len = 0;

	// TODO: Allow loading levels from $XDG_DATA_HOME
	char *levelDir = GAME_DATA_DIR"/levels/";

	DIR *d = opendir(levelDir);
	if (d == NULL) {
		platform_Log("can't open directory %s\n", levelDir);
		return;
	}
	struct dirent *f = NULL;
	char path[PATH_MAX];
	int levelDirLen = strlen(levelDir);
	memcpy(path, levelDir, levelDirLen);
	while ((f = readdir(d)) != NULL) {
		if (strcmp(f->d_name, ".") == 0 || strcmp(f->d_name, "..") == 0) {
			continue;
		}
		int len = strlen(f->d_name);
		memcpy(path+levelDirLen, f->d_name, len);
		*(path+levelDirLen+len) = 0;
		struct Level l = loadLevel(path);
		if (l.regions == NULL || l.regions_len == 0) {
			continue;
		}

		levels_len++;
		levels = erealloc(levels, levels_len * sizeof(*levels));
		levels[levels_len-1] = l;
	}
	closedir(d);

	game.levels = levels;
	game.levels_len = levels_len;
}

void
game_Init(void)
{
	struct game_Data data = {0};
	platform_ReadSaveData(&data);
	game.level = data.level;

	game.state = STATE_MENU;
	game.numplayers = 0;
	game.running = true;

	game.w = LOGICAL_WIDTH;
	game.h = LOGICAL_HEIGHT;

	initTileTextures();

	loadLevels();
	if (game.levels_len <= 0) {
		platform_Log("failed to load levels\n");
		exit(1);
	}

	player[0].proj.x = -1;
	player[0].proj.y = -1;
	player[1].proj.y = -1;
	player[1].proj.y = -1;

	game.screens[0].cam_x = 0;
	game.screens[0].cam_y = 0;
	game.screens[0].x = 0;
	game.screens[0].y = 0;
	game.screens[0].w = LOGICAL_WIDTH;
	game.screens[0].h = LOGICAL_HEIGHT;
}

void
game_Quit(void)
{
	freePlayerTextures();

	for (int i = 0; i < game.levels_len; i++) {
		free(game.levels[i].regions);
	}
	free(game.levels);
	game.levels_len = 0;

	struct game_Data data = {
		.level = game.level,
	};
	platform_WriteSaveData(&data);
}

static void
drwText(char *text, int x, int y, int size)
{
	struct game_Color fg = {
		.a = 0xFF,
		.r = 0xFF,
		.g = 0xFF,
		.b = 0xFF,
	};
	platform_RenderText(text, size, fg, x, y);
}

static void
drwTextScreenCentered(char *text, int size)
{
	int w = 0, h = 0;
	platform_MeasureText(text, size, &w, &h);
	int x = game.w / 2 - w / 2;
	int y = game.h / 2 - h / 2;

	drwText(text, x, y, size);
}

static void
drwHomeMenu(int gaps, int focus, int size, int width, int height)
{
	struct game_Rect options[3] = {
		{gaps, gaps + size, width, height},
		{gaps, gaps + size * 2, width, height},
		{gaps, gaps + size * 3, width, height},
	};

	int textSize = game.h * 36 / LOGICAL_HEIGHT;

	for (size_t i = 0; i < sizeof(options) / sizeof(options[0]); i++) {
		if ((int)i == focus) {
			platform_FillRect(GAME_RGB(20, 190, 180), &options[i]);
		} else {
			platform_FillRect(GAME_RGB(20, 150, 180), &options[i]);
		}
	}

	char nplayer[] = {game.numplayers + '1', '\0'};

	double offset = (double)height / 2.0;
	drwText(NAME, gaps, offset, textSize*2);

	offset += gaps;

	drwText("Start", gaps*2, offset + size, textSize);

	int w = 0;
	platform_MeasureText(nplayer, textSize, &w, NULL);

	drwText("Choose players", gaps*2, offset + size * 2, textSize);
	drwText(nplayer, width - w, offset + size * 2, textSize);

	drwText("Exit", gaps*2, offset + size * 3, textSize);
}

void
down(int i, bool keyReleased)
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
		player[i].dy -= JUMP_ACCEL;
		player[i].inAir = true;
	}
}

void
right(int i, bool keyReleased)
{

	if (player[i].x + player[i].w >= stage_length)
		return;

#if 0
	static uint32_t released = 0;
	if (keyReleased) {
		released = SDL_GetTicks();
		return;
	}

	if (released > 0) {
		uint32_t r = SDL_GetTicks();
		if (r - released < 200) {
			player[i].dx = SPEED_MAX * 2;
			player[i].current = &player[i].frame[1];
			return;
		}
		released = 0;
	}
#endif

	if (player[i].dx < SPEED_MAX)
		player[i].dx += SPEED_ACCEL;
	else
		player[i].dx = SPEED_MAX;

	player[i].current = &player[i].frame[1];
}

void
left(int i, bool keyReleased)
{
	if (player[i].x <= 0)
		return;

#if 0
	static uint32_t released = 0;
	if (keyReleased) {
		released = SDL_GetTicks();
		return;
	}

	if (released > 0) {
		uint32_t r = SDL_GetTicks();
		if (r - released < 200) {
			player[i].dx = -SPEED_MAX * 2;
			player[i].current = &player[i].frame[2];
			return;
		}
		released = 0;
	}
#endif

	if (player[i].dx < -SPEED_MAX)
		player[i].dx = -SPEED_MAX;
	else
		player[i].dx -= SPEED_ACCEL;

	player[i].current = &player[i].frame[2];
}

void
shoot(int i)
{
	if (player[i].proj.active) {
		return;
	}
	player[i].proj.x = player[i].x + player[i].w/2;
	player[i].proj.y = player[i].y + player[i].h/2;
	player[i].proj.initial_x = player[i].proj.x;
	player[i].proj.active = true;
	player[i].proj.retToPlayer = false;
	player[i].proj.dy = 0;

	// XXX: this seems really magical if you don't know that frame[2]
	// contains sprite for looking left.
	if (player[i].current == &player[i].frame[2]) {
		player[i].proj.dx = -PROJ_DX;
	} else {
		player[i].proj.dx = PROJ_DX;
	}
}

static double
playerVerticalCollision(int i)
{
	double dy = player[i].dy;

	/* Player isn't moving and collision detection is unnecessary */
	if (dy == 0)
		return player[i].y;

	struct game_FRect p = {
		(float)player[i].x,
		(float)player[i].y,
		(float)player[i].w,
		(float)player[i].h + dy,
	};

	struct Region *reg = NULL;
	struct Level *level = &game.levels[game.curLevel];
	for (size_t j = 0; j < level->regions_len; j++) {
		//if (level.regions[j].rect.x + level.regions[j].rect.w < p.x)
		//	continue;
		//if (p.x + p.w < level.regions[j].rect.x)
		//	continue;

		struct game_FRect r = {
			.x = (float)level->regions[j].rect.x,
			.y = (float)level->regions[j].rect.y,
			.w = (float)level->regions[j].rect.w,
			.h = (float)level->regions[j].rect.h,
		};
		// TODO: this code will allow for clipping, we should go
		// through all the regions and choose the one in between the
		// initial and final postition of player and choose the
		// platform that's closest to the initial position of player.
		if (game_HasIntersectionF(p, r)) {
			if (reg != NULL) {
				//if reg->rect.y is farther from p.y than r.y {
				//	reg = &level.regions[j];
				//}
			} else {
				reg = &level->regions[j];
			}
		}
	}


	if (reg != NULL) {
		if (reg->t == endPointTexture) {
			game.state = STATE_WON;
			return player[i].y;
		}
		player[i].dy = 0;
		if (dy > 0) { /* Player hit a platform while falling */
			player[i].inAir = false;
			return (float)reg->rect.y - player[i].h;
		} else if (dy < 0) { /* Player bonked while jumping */
			return (float)reg->rect.y + (float)reg->rect.h;
		}
	}


	if (dy > 0) {
		player[i].inAir = true;
	}

	return player[i].y + dy;
}

static void
movePlayerVertical(int i, double dt)
{
	if (player[i].dy > TERMINAL_VELOCITY) {
		player[i].dy = TERMINAL_VELOCITY;
	}

	player[i].y = playerVerticalCollision(i);
	player[i].dy += GRAVITY * dt;

	if (player[i].y > stage_height) {
		game.state = STATE_DEAD;
	}
}

static double
playerHorizontalCollision(int i)
{
	double dx = player[i].dx;

	/* Player isn't moving and collision detection is unnecessary */
	if (dx == 0)
		return player[i].x;

	struct game_FRect p = {
		(float)player[i].x,
		(float)player[i].y,
		(float)player[i].w + dx,
		(float)player[i].h
	};

	struct Level *level = &game.levels[game.curLevel];
	for (size_t j = 0; j < level->regions_len; j++) {
		struct game_FRect r = {
			.x = (float)level->regions[j].rect.x,
			.y = (float)level->regions[j].rect.y,
			.w = (float)level->regions[j].rect.w,
			.h = (float)level->regions[j].rect.h,
		};
		// TODO: this code will allow for clipping, we should go
		// through all the regions and choose the one in between the
		// initial and final postition of player and choose the
		// platform that's closest to the initial position of player.
		if (game_HasIntersectionF(p, r)) {
			if (level->regions[j].t == endPointTexture) {
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
moveCameras(void)
{
	if (split_screen) {
		for (int i = 0; i <= game.numplayers; i++) {
			double *cam_x = &game.screens[i].cam_x;
			double *cam_y = &game.screens[i].cam_y;
			double width = game.screens[i].w;
			double height = game.screens[i].h;
			double avg_x = player[i].x;
			if (avg_x > width / 2 && avg_x < stage_length - width / 2) {
				if (avg_x - *cam_x < -0.9 || avg_x - *cam_x > 0.9) {
					*cam_x = avg_x - (width / 2);
				}
			}

			double avg_y = player[i].y;;
			*cam_y = avg_y - height / 2;
			if (*cam_y > stage_height - width) {
				*cam_y = stage_height - width;
			}
		}
	} else {
		double *cam_x = &game.screens[0].cam_x;
		double *cam_y = &game.screens[0].cam_y;
		double width = game.screens[0].w;
		double height = game.screens[0].h;
		double total_x = 0;
		double total_y = 0;
		for (int i = 0; i <= game.numplayers; i++) {
			total_x += player[i].x;
			total_y += player[i].y;
		}

		double avg_x = total_x / (double)(game.numplayers + 1);
		if (avg_x > width / 2 && avg_x < stage_length - width / 2) {
			if (avg_x - *cam_x < -0.9 || avg_x - *cam_x > 0.9) {
				*cam_x = avg_x - (width / 2);
			}
		}

		double avg_y = total_y / (double)(game.numplayers + 1);
		*cam_y = avg_y - height / 2;
		if (*cam_y > stage_height - width) {
			*cam_y = stage_height - width;
		}
	}
}

static void
movePlayerProjectile(int i, float dt)
{
	if (!player[i].proj.active) {
		return;
	}
	if (player[i].proj.retToPlayer) {
		struct game_FRect pl = {
			player[i].x,
			player[i].y,
			player[i].w,
			player[i].h
		};
		struct game_FRect pr = {
			player[i].proj.x,
			player[i].proj.y,
			PROJ_SIZE,
			PROJ_SIZE,
		};
		if (game_HasIntersectionF(pl, pr)) {
			player[i].proj.active = false;
			player[i].proj.retToPlayer = false;
		} else {
			// XXX: maybe move in a curve to the
			// player instead of a straight line
			float eps = 4;
			if (player[i].x - player[i].proj.x > eps) {
				player[i].proj.dx = PROJ_DX;
			} else if (player[i].x - player[i].proj.x < -eps) {
				player[i].proj.dx = -PROJ_DX;
			} else {
				player[i].proj.dx = 0;
			}
			if (player[i].y - player[i].proj.y > eps) {
				player[i].proj.dy = PROJ_DX;
			} else if (player[i].y - player[i].proj.y < -eps) {
				player[i].proj.dy = -PROJ_DX;
			} else {
				player[i].proj.dy = 0;
			}
		}
	} else if (fabs(player[i].proj.x-player[i].proj.initial_x) > PROJ_RET) {
		player[i].proj.retToPlayer = true;
	}

	player[i].proj.x += player[i].proj.dx * dt;
	player[i].proj.y += player[i].proj.dy * dt;

}

static void
movePlayerHorizontal(int i, float dt)
{
	if ((player[i].x <= 0 && player[i].dx < 0)
			|| (player[i].x + player[i].w >= stage_length
				&& player[i].dx > 0)) {
		player[i].dx = 0;
	}

	player[i].dx = player[i].dx * FRICTION * dt;
	player[i].x = playerHorizontalCollision(i);

	if (player[i].x + player[i].w >= stage_length || player[i].x <= 0) {
		player[i].dx = 0;
	}
}

static void
movePlayers(float dt)
{
	/* TODO: Player-Player collision */
	for (int i = 0; i <= game.numplayers; i++) {
		movePlayerVertical(i, dt);
		movePlayerHorizontal(i, dt);
		movePlayerProjectile(i, dt);
	}

	moveCameras();
}

static void
drawPlayer(int i, double x, double y, double w, double h, double cam_x, double cam_y)
{
	struct game_Rect playRect = {
		x + player[i].x - cam_x,
		y + player[i].y - cam_y,
		player[i].w,
		player[i].h,
	};

	if (playRect.x + playRect.w  < x || playRect.x > x + w) {
		return;
	}
	if (playRect.y + playRect.h  < y || playRect.y > y + h) {
		return;
	}

	/* Draw a black square in place of texture that failed to load */
	if (*player[i].current == NULL) {
		if (playRect.x + playRect.w > x + w) {
			playRect.w = x + w - playRect.x;
		}
		if (playRect.x < x) {
			playRect.w -= x - playRect.x;
			playRect.x = playRect.x;
		}
		platform_FillRect(GAME_RGB(0, 0, 0), &playRect);
	} else {
		struct game_Rect s = {
			.x = 0,
			.y = 0,
			.w = player[i].w,
			.h = player[i].h,
		};
		struct game_Rect p = playRect;
		if (p.x + p.w > x + w) {
			p.w = x + w - p.x;
		}
		if (p.x < x) {
			p.w -= x - p.x;
			p.x = x;
		}

		s.w = s.w * p.w / playRect.w;
		s.x = p.x - playRect.x;

		platform_DrawTexture(*player[i].current, &s, &p);
	}

	if (player[i].proj.active &&
			player[i].proj.x - cam_x < w &&
			player[i].proj.y - cam_y < h &&
			player[i].proj.x - cam_x >= 0 &&
			player[i].proj.y - cam_y >= 0) {
		struct game_Rect projRect = {
			x + player[i].proj.x - cam_x,
			y + player[i].proj.y - cam_y,
			PROJ_SIZE,
			PROJ_SIZE,
		};

		// XXX: have a spinning animation?
		platform_FillRect(GAME_RGB(0XC8, 0XD6, 0XFF), &projRect);
	}
}

static void
drwPlayers(void)
{
	for (int j = 0; j <= game.numplayers; j++) {
		double cam_y = game.screens[j].cam_y;
		double cam_x = game.screens[j].cam_x;
		double x = game.screens[j].x;
		double y = game.screens[j].y;
		double w = game.screens[j].w;
		double h = game.screens[j].h;
		for (int i = 0; i <= game.numplayers; i++) {
			if (i == j) {
				continue;
			}
			drawPlayer(i, x, y, w, h, cam_x, cam_y);
		}
		// TODO: change the camera so player can always be seen.
		drawPlayer(j, x, y, w, h, cam_x, cam_y);
	}
}

static void
drawPlatform(game_Texture *t, struct game_Rect rect, struct game_Rect screen)
{
	for (int y = rect.y; y < rect.y + rect.h; y += BLOCK_SIZE) {
		if (y + BLOCK_SIZE < 0 || y > screen.h) {
			continue;
		}
		struct game_Rect d = rect;
		d.h = BLOCK_SIZE;
		if (y + d.h > screen.h) {
			d.h = screen.h - y;
		}
		d.y = y + screen.y;
		if (d.y < screen.y) {
			d.h -= screen.y - d.y;
			d.y = screen.y;
		}
		for (int x = rect.x; x < rect.x + rect.w; x += BLOCK_SIZE) {
			if (x + BLOCK_SIZE < 0 || x > screen.w) {
				continue;
			}
			d.w = BLOCK_SIZE;
			if (x + d.w > screen.w) {
				d.w = screen.w - x;
			}
			d.x = x + screen.x;
			if (d.x < screen.x) {
				d.w -= screen.x - d.x;
				d.x = screen.x;
			}

			// TODO: this isn't quite it, but good enough for now.
			struct game_Rect s = {
				.x = 0,
				.y = 0,
				.w = d.w,
				.h = d.h,
			};
			platform_DrawTexture(t, &s, &d);
		}
	}
}

static void
drwPlatforms(void)
{
	struct Level *level = &game.levels[game.curLevel];
	for (size_t i = 0; i < level->regions_len; i++) {
		for (int j = 0; j <= game.numplayers; j++) {
			double *cam_y = &game.screens[j].cam_y;
			double *cam_x = &game.screens[j].cam_x;
			struct game_Rect r = {
				.x = level->regions[i].rect.x - *cam_x,
				.y = level->regions[i].rect.y - *cam_y,
				.w = level->regions[i].rect.w,
				.h = level->regions[i].rect.h,
			};
			struct game_Rect dst = {
				.x = game.screens[j].x,
				.y = game.screens[j].y,
				.w = game.screens[j].w,
				.h = game.screens[j].h,
			};

			if (r.x > game.screens[j].w || r.y > game.screens[j].h) {
				continue;
			}

			drawPlatform(level->regions[i].t, r, dst);
		}
	}
}

void
drw(void)
{
	static int death_alpha = 0;
	if (game.state != STATE_DEAD) death_alpha = 0;

	platform_Clear(GAME_RGB(0, 0, 0));
	switch (game.state) {
	case STATE_MENU:;
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
				game.running = false;
				break;
			}
		}
		game.focusSelect = 0;

		drwHomeMenu(gaps, game.menuFocus,
					sectionSize, sectionWidth, sectionHeight);
		break;
	case STATE_PLAY:
		/* Change background to color: "#114261" */
		platform_FillRect(GAME_RGB(0x11, 0x41, 0x61), NULL);

		movePlayers(game.dt);

		drwPlatforms();
		drwPlayers();

		if (game.numplayers > 0 && split_screen) {
			platform_DrawLine(GAME_BLACK, game.screens[0].w, game.screens[0].h,
					game.screens[1].x, game.screens[1].y);
		}
		break;
	case STATE_WON: {
		// TODO: menu or at least keys to restart the level or go back
		// to main menu
		drwTextScreenCentered("You won", 40);
		break;
	}
	case STATE_DEAD: {
		// TODO: menu or at least keys to restart the level or go back
		// to main menu
		if (death_alpha < 255) {
			platform_FillRect(GAME_RGB(0x11, 0x41, 0x61), NULL);
			drwPlatforms();
			drwPlayers();

			platform_FillRect(GAME_RGBA(0, 0, 0, death_alpha), NULL);
		}

		drwTextScreenCentered("You died", 40);

		if (death_alpha < 255)
			death_alpha += 2;

		if (death_alpha > 255)
			death_alpha = 255;

		break;
	}
	case STATE_PAUSE:
		platform_FillRect(GAME_RGB(0x11, 0x41, 0x61), NULL);

		drwPlatforms();
		drwPlayers();

		// TODO: a pause menu
		struct game_Rect r = {
			.x = game.w / 2 - BLOCK_SIZE,
			.y = game.h / 2 - 2 * BLOCK_SIZE,
			.w = BLOCK_SIZE,
			.h = BLOCK_SIZE*4,
		};
		platform_FillRect(GAME_RGB(0x11, 0x11, 0x11), &r);
		r.x = game.w / 2 + BLOCK_SIZE;
		platform_FillRect(GAME_RGB(0x11, 0x11, 0x11), &r);
		break;
	}
}

static void
handleKey(int sym)
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
			game.running = false;
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

static void
handleKeyRelease(int sym, int player)
{
	switch (game.state) {
	case STATE_WON:  /* FALLTHROUGH */
	case STATE_DEAD: /* FALLTHROUGH */
	case STATE_MENU: /* FALLTHROUGH */
	case STATE_PAUSE:
		break;
	case STATE_PLAY:
		player = player > game.numplayers ? 0 : player;
		switch (sym) {
		case KEY_UP:
			jump(player, true);
			break;
		case KEY_RIGHT:
			right(player, true);
			break;
		case KEY_LEFT:
			left(player, true);
			break;
		}
		break;
	}
}

static void
handleKeyRepeat(int sym, int player)
{
	switch (game.state) {
	case STATE_WON:  /* FALLTHROUGH */
	case STATE_DEAD: /* FALLTHROUGH */
	case STATE_MENU: /* FALLTHROUGH */
	case STATE_PAUSE:
		break;
	case STATE_PLAY:
		player = player > game.numplayers ? 0 : player;
		switch (sym) {
		case KEY_UP:
			jump(player, false);
			break;
		case KEY_DOWN:
			down(player, false);
			break;
		case KEY_LEFT:
			left(player, false);
			break;
		case KEY_RIGHT:
			right(player, false);
			break;
		case KEY_SHOOT:
			shoot(player);
			break;
		case KEY_PAUSE:
		case KEY_QUIT:
			break;
		};
		break;
	};
}

static void
game_Draw(double dt, int width, int height)
{
	game.dt = dt;
	game.w = width;
	game.h = height;
	drw();
}

static void
game_Update(struct game_Input input, double dt, int width, int height)
{
	for (int player = 0; player < 2; player++) {
		for (int i = 0; i < KEY_COUNT; i++) {
			switch (input.keys[player][i]) {
			case KEY_UNKNOWN:
				break;
			case KEY_PRESSED:
				handleKey(i);
				if (game.state == STATE_PLAY) {
					handleKeyRepeat(i, player);
				}
				break;
			case KEY_PRESSED_REPEAT:
				handleKeyRepeat(i, player);
				break;
			case KEY_RELEASED:
				handleKeyRelease(i, player);
				break;
			}
		}
	}
}

bool
game_UpdateAndDraw(double dt, struct game_Input input, int width, int height)
{
	game.screens[0].x = 0;
	game.screens[0].y = 0;
	game.screens[0].w = width;
	game.screens[0].h = height;
	if (game.numplayers == 1) {
		game.screens[0].w = width/2;
	}
	game.screens[1].x = width/2;
	game.screens[1].y = 0;
	game.screens[1].w = width/2;
	game.screens[1].h = height;

	game_Update(input, dt, width, height);
	game_Draw(dt, width, height);
	return game.running;
}

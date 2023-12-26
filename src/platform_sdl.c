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
#include <cairo.h>
#include <SDL.h>
#include <stdbool.h>
#include <stdio.h>
#include <limits.h>
#include <unistd.h>

#include "fuyunix.h"
#include "game.h"
#include "scfg.h"
#include "util.h"

static SDL_Window *window;
static SDL_Renderer *renderer;

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
platform_Init(int flags)
{
	negativeDie(SDL_Init(SDL_INIT_VIDEO));

	window = nullDie(SDL_CreateWindow(NAME, SDL_WINDOWPOS_UNDEFINED,
				SDL_WINDOWPOS_UNDEFINED, LOGICAL_WIDTH,
				LOGICAL_HEIGHT, flags));
	renderer = nullDie(SDL_CreateRenderer(window, -1,
				SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC));

	SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
}

static void
platform_Quit(void)
{
	SDL_DestroyRenderer(renderer);
	SDL_DestroyWindow(window);
	SDL_Quit();
}

struct Key {
	SDL_Scancode key;
	KeySym sym;
	int player;
};

static struct {
	struct Key *key;
	int keylen;
	int is_key_allocated;
} keys;

_Static_assert(KEY_COUNT == 8, "Update keyList");

static char *keysList[] = {
	[KEY_UP]     = "up",
	[KEY_DOWN]   = "down",
	[KEY_LEFT]   = "left",
	[KEY_RIGHT]  = "right",
	[KEY_PAUSE]  = "pause",
	[KEY_SHOOT]  = "shoot",
	[KEY_QUIT]   = "quit",
};

static int
stringToKey(char *s)
{
	for (size_t i = 0; i < sizeof(keysList) / sizeof(keysList[0]); i++) {
		if (strcmp(s, keysList[i]) == 0)
			return i;
	}
	return -1;
}

static struct Key defaultkey[] = {
	{SDL_SCANCODE_Q,        KEY_QUIT,   0},
	{SDL_SCANCODE_ESCAPE,   KEY_PAUSE,  0},

	/* Player 1 */
	{SDL_SCANCODE_H,        KEY_LEFT,   0},
	{SDL_SCANCODE_J,        KEY_DOWN,   0},
	{SDL_SCANCODE_K,        KEY_UP,     0},
	{SDL_SCANCODE_L,        KEY_RIGHT,  0},
	{SDL_SCANCODE_U,        KEY_SHOOT,  0},
	/* Player 2 */
	{SDL_SCANCODE_A,        KEY_LEFT,   1},
	{SDL_SCANCODE_S,        KEY_DOWN,   1},
	{SDL_SCANCODE_W,        KEY_UP,     1},
	{SDL_SCANCODE_D,        KEY_RIGHT,  1},
	{SDL_SCANCODE_E,        KEY_SHOOT,  1},
};

void
freeKeys(void)
{
	if (keys.is_key_allocated)
		free(keys.key);
}

// XXX: it is possible to share most of this code with the wayland version.
void
loadConfig(void)
{
	char filepath[PATH_MAX];
	struct Key *keylist = NULL;
	int keylist_len = 0;
	struct scfg_block block;

	if (!getConfigFile(filepath, sizeof(filepath))) {
		goto default_config;
	}
	if (scfg_load_file(&block, filepath) < 0) {
		perror(filepath);
		goto default_config;
	}

	for (size_t i = 0; i < block.directives_len; i++) {
		struct scfg_directive *d = &block.directives[i];
		if (strcmp(d->name, "keys") == 0) {
			if (d->params_len > 0) {
				fprintf(stderr, "%s:%d: Expected 0 params got %zu params\n",
						filepath, d->lineno, d->params_len);
				exit(1);
			}
			struct scfg_block *child = &d->children;

			for (size_t i = 0; i < child->directives_len; i++) {
				struct Key key;
				struct scfg_directive *d = &child->directives[i];
				if (d->params_len != 2) {
					fprintf(stderr, "%s:%d: Expected 2 params, got %zu\n",
							filepath, d->lineno, d->params_len);
					exit(1);
				};
				int sym = stringToKey(d->name);
				if (sym < 0) {
					fprintf(stderr, "%s:%d: Invalid directive %s\n", filepath,
							d->lineno, d->name);
					exit(1);
				}
				SDL_Scancode keycode = SDL_GetScancodeFromName(d->params[0]);
				if (keycode == SDL_SCANCODE_UNKNOWN) {
					fprintf(stderr, "%s:%d: Invalid name %s\n",
							filepath, d->lineno, d->params[0]);
					exit(1);
				}
				int player = atoi(d->params[1]);
				if (player <= 0) {
					fprintf(stderr, "%s:%d: Invalid number %s\n",
							filepath, d->lineno, d->params[1]);
					exit(1);
				}

				key.sym = sym;
				key.key = keycode;
				key.player = player-1;

				keylist_len++;
				keylist = erealloc(keylist, keylist_len * sizeof(struct Key));
				keylist[keylist_len-1] = key;
			}
		}
	}

	scfg_block_finish(&block);

	if (keylist != NULL) {
		keys.key = keylist;
		keys.keylen = keylist_len;
		keys.is_key_allocated = 1;
	} else
default_config:
	{
		keys.key = defaultkey;
		keys.keylen = sizeof(defaultkey) / sizeof(defaultkey[0]);
		keys.is_key_allocated = 0;
	}
}

void
listFunc(void)
{
	for (size_t i = 0; i < sizeof(keysList) / sizeof(keysList[0]); i++)
		printf("%s\n", keysList[i]);
}

int
gameKey(SDL_Scancode code, int *p)
{
	for (int i = 0; i < keys.keylen; i++) {
		if (keys.key[i].key == code) {
			*p = keys.key[i].player;
			return keys.key[i].sym;
		}
	}
	*p = 0;
	switch (code) {
	case SDL_SCANCODE_UP:
		return KEY_UP;
	case SDL_SCANCODE_DOWN:
		return KEY_DOWN;
	case SDL_SCANCODE_LEFT:
		return KEY_LEFT;
	case SDL_SCANCODE_RIGHT:
		return KEY_RIGHT;
	case SDL_SCANCODE_Q:
		return KEY_QUIT;
	case SDL_SCANCODE_ESCAPE:
		return KEY_PAUSE;
	case SDL_SCANCODE_RETURN: // fallthrough
	case SDL_SCANCODE_SPACE:
		return KEY_SELECT;
	default:
		return -1;
	}
}

static void
run(void)
{
	SDL_Event event;
	uint32_t t = SDL_GetTicks();
	struct game_Input input = {0};
	while (true) {
		while (SDL_PollEvent(&event) != 0) {
			if (event.type == SDL_QUIT)
				return;

			int player = 0;
			int key = gameKey(event.key.keysym.scancode, &player);
			if (key < 0)
				continue;

			if (event.type == SDL_KEYDOWN)
				input.keys[player][key] = KEY_PRESSED;
			else if (event.type == SDL_KEYUP)
				input.keys[player][key] = KEY_RELEASED;
		}

		uint32_t nt = SDL_GetTicks();
		double dt = (double)(nt - t) / 1000.0;
		t = nt;
		int width, height;
		SDL_GetWindowSize(window, &width, &height);

		static int prevWidth = 0, prevHeight = 0;
		static cairo_surface_t *csurf = NULL;
		static cairo_t *cr = NULL;
		static void *framebuffer = NULL;

		if (prevWidth != width || prevHeight != height) {
			prevWidth = width;
			prevHeight = height;

			if (csurf != NULL) {
				cairo_surface_finish(csurf);
				cairo_surface_destroy(csurf);
			}
			if (cr != NULL) {
				cairo_destroy(cr);
			}

			framebuffer = erealloc(framebuffer, width * height * 32);
			csurf = cairo_image_surface_create_for_data(
					(unsigned char *)framebuffer, CAIRO_FORMAT_ARGB32,
					width, height, width*4);
			if (cairo_surface_status(csurf) != CAIRO_STATUS_SUCCESS) {
				fprintf(stderr, "cairo: %s\n",
						cairo_status_to_string(cairo_surface_status(csurf)));
				exit(1);
			}
			cr = cairo_create(csurf);
			if (cairo_status(cr) != CAIRO_STATUS_SUCCESS) {
				fprintf(stderr, "cairo: %s\n",
						cairo_status_to_string(cairo_status(cr)));
				exit(1);
			}
		}

		if (!game_UpdateAndDraw(cr, dt, input, width, height)) {
			return;
		}
		cairo_surface_flush(csurf);
		SDL_Surface *surf =
			nullDie(SDL_CreateRGBSurfaceFrom(framebuffer, width,
						height, 32, width * 4,
						0x00FF0000,  0x0000FF00,
						0x000000FF, 0xFF000000));
		SDL_Texture *texture = nullDie(SDL_CreateTextureFromSurface(renderer, surf));
		SDL_RenderCopy(renderer, texture, NULL, NULL);
		SDL_RenderPresent(renderer);
		SDL_DestroyTexture(texture);
		SDL_FreeSurface(surf);

		for (int player = 0; player < 2; player++) {
			for (int i = 0; i < KEY_COUNT; i++) {
				switch (input.keys[player][i]) {
				case KEY_PRESSED:
					input.keys[player][i] = KEY_PRESSED_REPEAT;
					break;
				case KEY_RELEASED:
					input.keys[player][i] = KEY_UNKNOWN;
					break;
				default: // ignore
					break;
				}
			}
		}
	}
}

int
main(int argc, char *argv[])
{
	int x;
	int flags = SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE;

	if (argc > 1) {
		while ((x = getopt(argc, argv, "vlf")) != -1) {
			switch (x) {
			case 'v':
				puts(NAME": " VERSION);
				return 0;
			case 'l':
				listFunc();
				return 0;
			case 'f':
				flags |= SDL_WINDOW_FULLSCREEN;
				break;
			default:
				fputs("Usage: fuyunix [-v|-l|-f]\n", stderr);
				return 1;
			}
		}
	}

	platform_Init(flags);

	loadConfig();

	game_Init();
	run();
	game_Quit();

	platform_Quit();

	return 0;
}

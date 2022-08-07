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
#include <SDL.h>
#include <stdio.h>
#include <stdlib.h>

#include "drw.h"
#include "keys.h"
#include "scfg.h"
#include "util.h"

struct Key {
	SDL_Scancode key;
	enum KeySym sym;
	int player;
};

static struct {
	struct Key *key;
	int keylen;
	int is_key_allocated;
} keys;

_Static_assert(KEY_COUNT == 7, "update keyList");
static char *keysList[] = {
	[KEY_UP]     = "up",
	[KEY_DOWN]   = "down",
	[KEY_LEFT]   = "left",
	[KEY_RIGHT]  = "right",
	[KEY_PAUSE]  = "pause",
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

	/* Player 2 */
	{SDL_SCANCODE_A,        KEY_LEFT,   1},
	{SDL_SCANCODE_S,        KEY_DOWN,   1},
	{SDL_SCANCODE_W,        KEY_UP,     1},
	{SDL_SCANCODE_D,        KEY_RIGHT,  1},
};

void
menuHandleKeys(int scancode)
{
	switch (scancode) {
	case SDL_SCANCODE_UP:
		menuHandleKey(KEY_UP);
		return;
	case SDL_SCANCODE_DOWN:
		menuHandleKey(KEY_DOWN);
		return;
	case SDL_SCANCODE_Q:
		menuHandleKey(KEY_QUIT);
		return;
	case SDL_SCANCODE_ESCAPE:
		menuHandleKey(KEY_PAUSE);
		return;
	case SDL_SCANCODE_RETURN:
	case SDL_SCANCODE_SPACE:
		menuHandleKey(KEY_SELECT);
		return;
	}
	for (int i = 0; i < keys.keylen; i++) {
		if (keys.key[i].key == (SDL_Scancode)scancode)
			menuHandleKeys(keys.key[i].sym);
	}
}

static const Uint8 *keyboardState = NULL;

void
handleKeys(void)
{
	for (int i = 0; i < keys.keylen; i++) {
		if (keyboardState[keys.key[i].key])
			handleKey(keys.key[i].sym, keys.key[i].player);
	}
}

void
freeKeys(void)
{
	if (keys.is_key_allocated)
		free(keys.key);
}

void
loadConfig(void)
{
	keyboardState = SDL_GetKeyboardState(NULL);

	char filepath[PATH_MAX];
	struct Key *keylist = NULL;
	int keylist_len = 0;
	struct scfg_block block;

	getPath(filepath, "XDG_CONFIG_HOME", "/config", 0);
	if (scfg_load_file(&block, filepath) < 0) {
		perror(filepath);
		goto default_config;
	};

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
				};
				SDL_Scancode keycode = SDL_GetScancodeFromName(d->params[0]);
				if (keycode == SDL_SCANCODE_UNKNOWN) {
					fprintf(stderr, "%s:%d: Invalid name %s\n",
							filepath, d->lineno, d->params[0]);
					exit(1);
				};
				int player = atoi(d->params[1]);
				if (player <= 0) {
					fprintf(stderr, "%s:%d: Invalid number %s\n",
							filepath, d->lineno, d->params[1]);
					exit(1);
				};

				key.sym = sym;
				key.key = keycode;
				key.player = player-1;

				keylist_len++;
				keylist = erealloc(keylist, keylist_len * sizeof(struct Key));
				keylist[keylist_len-1] = key;
			}
		} else if (strcmp(d->name, "sprite-dir") == 0) {
			// TODO: implement searching for sprites in user given directory
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
	for (int i = 0; i < (int)(sizeof(keysList) / sizeof(keysList[0])); i++)
		printf("%s\n", keysList[i]);
}

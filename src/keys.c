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

#include <SDL.h>

#include "drw.h"
#include "file.h"
#include "fuyunix.h"

typedef void (*FuncPtr)(int);

struct FuncValMap {
	char *name;
	FuncPtr func;
};

struct Key {
	SDL_Scancode key;
	FuncPtr func;
	int player;
};

struct Keys {
	struct Key *key;
	unsigned int keysize;
	int is_key_allocated;
};

struct FuncValMap funclist[] = {
	{ "jump",          jump },
	{ "down",          down },
	{ "right",        right },
	{ "left",          left },
	{ "quit",      quitloop },
	{ "drwmenu",    drwMenu },
};

static struct Key defaultkey[] = {
	{SDL_SCANCODE_Q,          quitloop,    0},
	{SDL_SCANCODE_ESCAPE,     drwMenu,     0},

	/* Player 1 */
	{SDL_SCANCODE_H,             left,     0},
	{SDL_SCANCODE_J,             down,     0},
	{SDL_SCANCODE_K,             jump,     0},
	{SDL_SCANCODE_L,            right,     0},

	/* Player 2 */
	{SDL_SCANCODE_A,             left,     1},
	{SDL_SCANCODE_S,             down,     1},
	{SDL_SCANCODE_W,             jump,     1},
	{SDL_SCANCODE_D,            right,     1},
};

/* Global variables */
static SDL_KeyboardEvent *keyevent;
struct Keys *keys;

/* Functions definitions */
FuncPtr
getFunc(char *name)
{
	size_t size = sizeof(funclist) / sizeof(funclist[0]);
	for (size_t i = 0; i < size; i++) {
		if (strcmp(funclist[i].name, name) == 0)
			return funclist[i].func;
	}

	return NULL;
}

static void
loopkeys(void)
{
	size_t i;
	for (i = 0; i < keys->keysize; i++) {
		if (keyevent->keysym.scancode == keys->key[i].key) {
			keys->key[i].func(keys->key[i].player);
		}

		const Uint8 *state = SDL_GetKeyboardState(NULL);
		if (left == keys->key[i].func && state[keys->key[i].key])
			keys->key[i].func(keys->key[i].player);
		if (right == keys->key[i].func && state[keys->key[i].key])
			keys->key[i].func(keys->key[i].player);
		if (down == keys->key[i].func && state[keys->key[i].key])
			keys->key[i].func(keys->key[i].player);
		if (jump == keys->key[i].func && state[keys->key[i].key])
			keys->key[i].func(keys->key[i].player);
	}
}

int
handleMenuKeys(int *focus, int last)
{
	SDL_Event event;

	while (SDL_PollEvent(&event) != 0) {
		if (event.type == SDL_QUIT) {
			quitloop();
			*focus = last;
			return 1;
		} else if (event.type == SDL_KEYDOWN) {
			switch (event.key.keysym.scancode) {
			case SDL_SCANCODE_K:
				if (*focus > 0)
					*focus -= 1;
				break;
			case SDL_SCANCODE_J:
				if (*focus < last)
					*focus += 1;
				break;
			case SDL_SCANCODE_Q:
				quitloop();
				*focus = last;
				return 1;
			case SDL_SCANCODE_RETURN:
			case SDL_SCANCODE_SPACE:
				return 1;
				// gcc / clang needs default:
			default:
				break;
			}
		}
	}
	return 0;
}

void
handleKeys(void *ptr)
{
	keyevent = ptr;

	loopkeys();
}
void
skipWhitespace(char *c, int *i)
{
	while (c[*i] == ' ' || c[*i] == '\t' || c[*i] == '\n') {
		if (c[*i] == '\0')
			return;
		(*i)++;
	}
}

char *
getStr(char *c, int *i)
{
	if (!isalnum(c[*i]))
		return NULL;

	int prev = *i;

	while (isalnum(c[*i]))
			(*i)++;

	char *val = calloc(1 + (*i) - prev, sizeof(char));
	if (val == NULL) {
		perror("Unable to allocate memory");
		exit(1);
	}

	strncpy(val,  c + prev, (*i) - prev);

	return val;
}
char *
getStrNl(char *c, int *i)
{
	if (!isalnum(c[*i]))
		return NULL;

	int prev = *i;

	while (c[*i] != '\n' && c[*i] != '\0')
			(*i)++;

	char *val = calloc(1 + (*i) - prev, sizeof(char));
	if (val == NULL) {
		perror("Unable to allocate memory");
		exit(1);
	}

	strncpy(val,  c + prev, (*i) - prev);

	return val;
}

struct Key *
getKey(char *c, int *size)
{
	struct Key *key = NULL;
	*size = 0;

	unsigned int i = 0;

	while (c[i] != '\0') {
		int player = 0;
		char *tmp;
		char *func;
		char *keyname;
		skipWhitespace(c, &i);
		tmp = getStr(c, &i);
		if (tmp != NULL) {
			player = atoi(tmp);
			free(tmp);
		}

		skipWhitespace(c, &i);

		func = getStr(c, &i);

		skipWhitespace(c, &i);

		keyname = getStrNl(c, &i);
		if (func == NULL || keyname == NULL) {
			free(keyname);
			free(func);
			return NULL;
		}

		key = realloc(key, ((*size) + 1) * sizeof(struct Key));
		if (key == NULL) {
			perror("Unable to reallocate memory");
			exit(1);
		}

		skipWhitespace(c, &i);

		key[*size].player = player;
		key[*size].func = getFunc(func);
		key[*size].key = SDL_GetScancodeFromName(keyname);

		if (key[*size].func == NULL || key[*size].key == SDL_SCANCODE_UNKNOWN) {
			fprintf(stderr, "Invalid function `%s` or key `%s`:\nThe default"
					"keys will be used\n", func , keyname);
			free(key);
			*size = 0;
			free(keyname);
			free(func);
			return NULL;
		}

		*size += 1;
		free(keyname);
		free(func);
	}

	return key;
}

void
getKeys(void)
{
	keys = calloc(1, sizeof(struct Keys));
	if (keys == NULL) {
		perror("Unable to allocate memory");
		exit(1);
	}
	keys->key = NULL;
	keys->is_key_allocated = 1;

	char *c = readKeyConf();
	if (c != NULL)
		keys->key = getKey(c, &keys->keysize);

	free(c);

	if (keys->key == NULL) {
		keys->key = defaultkey;
		keys->keysize = sizeof(defaultkey) / sizeof(defaultkey[0]);
		keys->is_key_allocated = 0;
	}
}

void
freeKeys(void)
{
	if (keys->is_key_allocated) {
		free(keys->key);
	}
	free(keys);
}

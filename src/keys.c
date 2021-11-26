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

#include <stdio.h>
#include <stdlib.h>
#include <SDL.h>

#include "alloc.h"
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
	int keysize;
	int is_key_allocated;
};

struct Parser {
	char *filename;
	char *file;
	char c;
	int i;
	int lineno;
};

struct FuncValMap funclist[] = {
	{ "jump",          jump },
	{ "down",          down },
	{ "right",        right },
	{ "left",          left },
	{ "quit",      quitloop },
	{ "draw menu",    drwMenu },
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
loopkeys(SDL_KeyboardEvent *keyevent)
{
	int i;
	const Uint8 *state = SDL_GetKeyboardState(NULL);
	for (i = 0; i < keys->keysize; i++) {
		if (keyevent->keysym.scancode == keys->key[i].key) {
			keys->key[i].func(keys->key[i].player);
		}

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
			return 0;
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
				return 0;
			case SDL_SCANCODE_RETURN:
			case SDL_SCANCODE_SPACE:
				return 0;
			/* default required for enum */
			default:
				break;
			}
		}
	}
	return 1;
}

void
handleKeys(SDL_KeyboardEvent *keyevent)
{
	loopkeys(keyevent);
}

void
parserAdvance(struct Parser *p)
{
	if (p->c != '\0')
		p->i++;

	p->c = p->file[p->i];
}

void
skipWhitespace(struct Parser *p)
{
	while (p->c == ' ' || p->c == '\t' || p->c == '\n') {
		if (p->c == '\n')
			p->lineno++;

		parserAdvance(p);
	}
}

char *
getStr(struct Parser *p)
{
	skipWhitespace(p);

	if (!isalnum(p->c)) {
		fprintf(stderr, "%s: Unexpected character `%c` on line %d\n",
				p->filename, p->c, p->lineno);
		return NULL;
	}

	int prev = p->i;

	while (isalnum(p->c))
		parserAdvance(p);

	int size = p->i - prev;
	char *val = qcalloc(1 + size, sizeof(char));

	strncpy(val,  &p->file[prev], size);

	return val;
}

char *
getStrNl(struct Parser *p)
{
	skipWhitespace(p);

	if (!isalnum(p->c)) {
		fprintf(stderr, "%s: Unexpected character `%c` on line %d\n",
				p->filename, p->c, p->lineno);
		return NULL;
	}

	int prev = p->i;

	while (p->c != '\n' && p->c != '\0')
		parserAdvance(p);

	int size = p->i - prev;
	char *val = qcalloc(1 + size, sizeof(char));

	strncpy(val,  &p->file[prev], size);

	return val;
}

struct Key *
parseKeys(struct Parser *p, int *size)
{
	struct Key *key = NULL;

	while (p->c != '\0') {
		int player = 0;
		char *tmp;
		char *func;
		char *keyname;

		tmp = getStr(p);
		if (tmp == NULL) {
			free(key);
			return NULL;
		}
		player = atoi(tmp);
		free(tmp);

		func = getStr(p);
		if (func == NULL) {
			free(key);
			return NULL;
		}

		keyname = getStrNl(p);
		if (keyname == NULL) {
			free(func);
			free(key);
			return NULL;
		}

		key = qrealloc(key, ((*size) + 1) * sizeof(struct Key));

		key[*size].player = player;
		key[*size].func = getFunc(func);
		key[*size].key = SDL_GetScancodeFromName(keyname);

		if (key[*size].func == NULL || key[*size].key == SDL_SCANCODE_UNKNOWN) {
			fprintf(stderr, "Invalid function `%s` or key `%s` on line %d:\n"
					"The default keys will be used\n", func, keyname,
					p->lineno);

			*size = 0;
			free(key);
			free(keyname);
			free(func);
			return NULL;
		}

		(*size)++;
		free(keyname);
		free(func);

		/* catch '\0' appearing after newline */
		skipWhitespace(p);
	}

	return key;
}

void
getKeys(void)
{
	keys = qcalloc(1, sizeof(struct Keys));
	keys->key = NULL;
	keys->is_key_allocated = 1;
	keys->keysize = 0;

	struct Parser p;
	p.file = readKeyConf(&p.filename);
	p.lineno = 1;
	p.i = 0;
	if (p.file != NULL) {
		p.c = p.file[p.i];
		keys->key = parseKeys(&p, &keys->keysize);
	}

	free(p.file);
	free(p.filename);

	if (keys->key == NULL) {
		keys->key = defaultkey;
		keys->keysize = sizeof(defaultkey) / sizeof(defaultkey[0]);
		keys->is_key_allocated = 0;
	}
}

void
freeKeys(void)
{
	if (keys->is_key_allocated)
		free(keys->key);

	free(keys);
}

void
listFunc(void)
{
	for (int i = 0; i < (int)(sizeof(funclist) / sizeof(funclist[0])); i++)
		printf("%s\n", funclist[i].name);
}

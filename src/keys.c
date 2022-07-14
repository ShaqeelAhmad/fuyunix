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

#include "alloc.h"
#include "drw.h"
#include "file.h"
#include "fuyunix.h"

/* keys buffered on the stack before it's allocated to the heap */
#define KEY_BUF 20

/* max bytes for each field (separated by space) */
#define FIELD_LEN 30

typedef void (*keyFunc)(int);

struct Key {
	SDL_Scancode key;
	keyFunc func;
	int player;
};

struct Keys {
	struct Key *key;
	int keysize;
	int is_key_allocated;
};

struct Parser {
	char filename[PATH_MAX];
	char *file;
	char c;
	int i;
	int lineno;
};

struct FuncValMap {
	char *name;
	keyFunc func;
};

/* Note: don't use spaces in the name */
static struct FuncValMap funclist[] = {
	{ "jump",           jump },
	{ "down",           down },
	{ "right",         right },
	{ "left",           left },
	{ "quit",       quitloop },
	{ "drawMenu",    drwMenu },
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
static struct Keys *keys;

/* Functions definitions */
static keyFunc
getFunc(char *name)
{
	size_t size = sizeof(funclist) / sizeof(funclist[0]);
	for (size_t i = 0; i < size; i++) {
		if (strcmp(funclist[i].name, name) == 0)
			return funclist[i].func;
	}

	return NULL;
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
handleKeys(const Uint8 *state)
{
	for (int i = 0; i < keys->keysize; i++) {
		if (state[keys->key[i].key])
			keys->key[i].func(keys->key[i].player);
	}
}

static void
parserAdvance(struct Parser *p)
{
	if (p->c != '\0')
		p->i++;

	p->c = p->file[p->i];
}

static void
skipWhitespace(struct Parser *p)
{
	while (p->c == ' ' || p->c == '\t' || p->c == '\n') {
		if (p->c == '\n')
			p->lineno++;

		parserAdvance(p);
	}
}

static char *
getField(char *str, struct Parser *p, int (*cmp)(int))
{
	skipWhitespace(p);

	if (!cmp(p->c)) {
		fprintf(stderr, "%s:%d Unexpected character `%c`(0x%X)\n",
				p->filename, p->lineno, p->c, p->c);
		return NULL;
	}

	int prev = p->i;

	while (cmp(p->c) && p->c != '\0')
		parserAdvance(p);

	int size = p->i - prev;
	if (size > FIELD_LEN) {
		fprintf(stderr, "%s:%d Field may be too long: Make sure it's at most"
				" %d bytes\nFirst %d bytes of field: `%.*s`\n" , p->filename,
				p->lineno, FIELD_LEN, FIELD_LEN, FIELD_LEN, p->file+prev);
		return NULL;
	}

	strncpy(str,  &p->file[prev], size);

	return str;
}

static int
isNewline(int c)
{
	return c != '\n';
}

static struct Key *
parseKeys(struct Parser *p, int *size)
{
	struct Key keybuf[KEY_BUF];
	size_t keybufSize = 0;
	struct Key *key = NULL;

	while (p->c != '\0') {
		int player = 0;
		char playerNum[FIELD_LEN];
		char func[FIELD_LEN];;
		char keyname[FIELD_LEN];
		memset(playerNum, 0, FIELD_LEN);
		memset(func, 0, FIELD_LEN);
		memset(keyname, 0, FIELD_LEN);

		char *err;
		err = getField(playerNum, p, isalnum);
		if (err == NULL) {
			free(key);
			return NULL;
		}
		player = atoi(playerNum);

		err = getField(func, p, isalnum);
		if (err == NULL) {
			free(key);
			return NULL;
		}

		err = getField(keyname, p, isNewline);
		if (err == NULL) {
			free(key);
			return NULL;
		}

		/* key = qrealloc(key, ((*size) + 1) * sizeof(struct Key)); */

		keybuf[keybufSize].player = player;
		keybuf[keybufSize].func = getFunc(func);
		keybuf[keybufSize].key = SDL_GetScancodeFromName(keyname);

		if (keybuf[keybufSize].func == NULL) {
			fprintf(stderr, "%s:%d Invalid function `%s`\n"
					"The default keys will be used\n", p->filename, p->lineno,
					func);

			*size = 0;
			free(key);
			return NULL;
		}
		if (keybuf[keybufSize].func == SDL_SCANCODE_UNKNOWN) {
			fprintf(stderr, "%s:%d Invalid  key `%s`\n"
					"The default keys will be used\n", p->filename, p->lineno,
					keyname);

			*size = 0;
			free(key);
			return NULL;
		}

		keybufSize++;

		if (keybufSize >= KEY_BUF) {
			*size += keybufSize;
			key = qrealloc(key, sizeof(struct Key) * *size);
			memcpy(key, keybuf, sizeof(struct Key) * *size);
			keybufSize = 0;
		}

		/* catch '\0' appearing after newline */
		skipWhitespace(p);
	}
	*size += keybufSize;
	key = qrealloc(key, sizeof(struct Key) * *size);
	memcpy(key, keybuf, sizeof(struct Key) * *size);

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
	p.file = readKeyConf(p.filename);
	p.lineno = 1;
	p.i = 0;
	if (p.file != NULL) {
		p.c = p.file[p.i];
		keys->key = parseKeys(&p, &keys->keysize);
	}

	free(p.file);

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

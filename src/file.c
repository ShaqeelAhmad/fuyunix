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

#include <ctype.h>
#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#include "alloc.h"

#define SAVEDIR "XDG_STATE_HOME"

static char *
getPath(char *fullpath, char *xdg, char *file, int createDir)
{
	char *dirpath;
	char *dir = "/fuyunix";
	if ((dirpath = getenv(xdg)) == NULL) {
		if ((dirpath = getenv("HOME")) == NULL) {
			perror("Unable to get HOME");
			exit(1);
		}
		/* Assume user wants it in HOME when XDG variables are not set */
		dir = "/.fuyunix";
	}

	strcpy(fullpath, dirpath);
	strcat(fullpath, dir);

	if (createDir && mkdir(fullpath, 0755) < 0 && errno != EEXIST) {
		fprintf(stderr, "Unable to create directory `%s`: %s\n",
				fullpath, strerror(errno));
	}

	strcat(fullpath, file);

	return fullpath;
}

int
readSaveFile(void)
{
	int level;
	char savepath[PATH_MAX];
	FILE *fp;

	getPath(savepath, SAVEDIR, "/save", 1);
	fp = fopen(savepath, "rb");

	if (fp == NULL) {
		if (errno != ENOENT)
			fprintf(stderr, "Couldn't open savefile `%s`: %s\n",
					savepath, strerror(errno));
		fclose(fp);
		return 1; /* return default level, 1 */
	}

	fread(&level, sizeof(int), 1, fp);

	fclose(fp);

	return level;
}

void
writeSaveFile(int level)
{
	char savepath[PATH_MAX];
	FILE *fp;

	getPath(savepath, SAVEDIR, "/save", 1);

	fp = fopen(savepath, "wb+");

	if (fp == NULL) {
		fprintf(stderr, "Couldn't open savefile `%s`: %s\n",
				savepath, strerror(errno));
		fclose(fp);
		return;
	}

	fwrite(&level, sizeof(int), 1, fp);

	fclose(fp);
}

static char *
readFile(char *name)
{
	FILE *fp = fopen(name, "r");

	/* Ignore file not existing; config file should be created by the user */
	if (fp == NULL) {
		if (errno != ENOENT)
			perror(name);

		return NULL;
	}

	fseek(fp, 0, SEEK_END);

	long size = ftell(fp);
	if (size <= 0) {
		if (size < 0)
			perror(name);

		fclose(fp);
		return NULL;
	}

	fseek(fp, 0, SEEK_SET);

	char *c = qcalloc(size, sizeof(char));

	fread(c, sizeof(char), size, fp);

	fclose(fp);

	return c;
}

static int
isSpace(char c)
{
	return c == ' ' || c == '\t';
}

static int
removeComments(char *buf)
{
	char *ret = buf;
	char *s = buf;
	for (; *buf != '\0'; buf++, s++) {
		if (*buf == '#') {
			if (buf > ret && isSpace(*(buf-1))) {
				buf--;
				for (; *buf != '\0' && isSpace(*buf); buf--)
					s--;
			}
			for (; *buf != '\0' && *buf != '\n'; buf++);
		}
		*s = *buf;
	}

	*s = '\0';

	return s - ret;
}

char *
readKeyConf(char filename[PATH_MAX])
{
	getPath(filename, "XDG_CONFIG_HOME", "/keys.conf", 0);
	char *file = readFile(filename);

	if (file == NULL) {
		if (errno != ENOENT)
			fprintf(stderr, "Couldn't open file `%s`: %s\n", filename,
					strerror(errno));

		return NULL;
	}

	removeComments(file);

	return file;
}

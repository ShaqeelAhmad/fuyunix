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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#include "alloc.h"

#define SAVEDIR "XDG_STATE_HOME"

static char *
getPath(char *xdg, char *file, int createDir)
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

	char *fullpath = qcalloc(strlen(dir) + strlen(dirpath) + strlen(file) + 1,
			sizeof(char));

	strcpy(fullpath, dirpath);
	strcat(fullpath, dir);

	if (createDir && mkdir(fullpath, 0755) < 0 && errno != EEXIST)
		perror("Unable to create directory");

	strcat(fullpath, file);

	return fullpath;
}

int
readSaveFile(void)
{
	int level;
	char *savepath;
	FILE *fp;

	savepath  = getPath(SAVEDIR, "/save", 1);

	fp = fopen(savepath, "rb");
	free(savepath);

	if (fp == NULL)
		return 1; /* return default level, 1 */

	fread(&level, sizeof(int), 1, fp);

	fclose(fp);

	return level;
}

void
writeSaveFile(int level)
{
	char *savepath;
	FILE *fp;

	savepath  = getPath(SAVEDIR, "/save", 1);

	fp = fopen(savepath, "wb+");
	free(savepath);

	if (fp == NULL) {
		perror("Couldn't open savefile");
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
	if (fp == NULL)
		return NULL;

	fseek(fp, 0, SEEK_END);

	long size = ftell(fp);
	if (size == 0) {
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
		if (*buf == '#' && isSpace(*(buf - 1))) {
			buf--;
			for (; *buf != '\0' && isSpace(*buf); buf--)
				s--;
			for (; *buf != '\0' && *buf != '\n'; buf++);
		}
		if (*buf == '#')
			for (; *buf != '\0' && *buf != '\n'; buf++);

		*s = *buf;
	}

	*s = '\0';

	return s - ret;
}

char *
readKeyConf(void)
{
	char *filename = getPath("XDG_CONFIG_HOME", "/keys.conf", 0);
	char *file = readFile(filename);
	free(filename);

	if (file == NULL)
		return NULL;

	int len = removeComments(file);
	file = qrealloc(file, len * sizeof(char));

	return file;
}

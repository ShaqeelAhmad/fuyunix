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

#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#include "util.h"

char *
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
		exit(1);
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

	getPath(savepath, "XDG_STATE_HOME", "/save", 1);
	fp = fopen(savepath, "rb");

	if (fp == NULL) {
		if (errno != ENOENT)
			fprintf(stderr, "Couldn't open savefile `%s`: %s\n",
					savepath, strerror(errno));
		return 1; /* return default level, 1 */
	}

	size_t n = fread(&level, sizeof(int), 1, fp);
	if (n != 1) {
		fprintf(stderr, "Error: %s: only read %zu expected to read %zu\n",
				savepath, n, sizeof(int));
		fclose(fp);
		return 1; /* return default level, 1 */
	}

	fclose(fp);

	return level;
}

void
writeSaveFile(int level)
{
	char savepath[PATH_MAX];
	FILE *fp;

	getPath(savepath, "XDG_STATE_HOME", "/save", 1);

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

void *
ecalloc(size_t nmemb, size_t size)
{
	void *p = calloc(nmemb, size);
	if (p == NULL) {
		perror("Unable to allocate memory");
		exit(1);
	}

	return p;
}

void *
erealloc(void *ptr, size_t size)
{
	void *p = realloc(ptr, size);
	if (p == NULL) {
		perror("Unable to reallocate memory");
		exit(1);
	}

	return p;
}

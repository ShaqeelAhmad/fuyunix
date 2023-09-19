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
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#include "util.h"

#ifdef _WIN32
#	define HOME_DIR_ENV   "USERPROFILE"
#	define CONFIG_DIR_ENV "AppData"
#	define SAVE_DIR_ENV   "LocalAppData"
#	define ALT_CONFIG_DIR "/AppData/Roaming/"
#	define ALT_SAVE_DIR   "/AppData/Local/"
#else
#	define HOME_DIR_ENV   "HOME"
#	define CONFIG_DIR_ENV "XDG_CONFIG_HOME"
#	define SAVE_DIR_ENV   "XDG_STATE_HOME"
#	define ALT_CONFIG_DIR "/.config/"
#	define ALT_SAVE_DIR   "/.local/state/"
#endif

int
getConfigDir(char *path, size_t path_len)
{
	char *dir = getenv(CONFIG_DIR_ENV);
	size_t dirlen = 0;
	char  *s =        "/fuyunix/";
	size_t n = sizeof("/fuyunix/")-1;
	if (dir) {
		dirlen = strlen(dir);
	} else {
		dir = getenv(HOME_DIR_ENV);
		if (dir == NULL) {
#ifdef _WIN32
			perror("Unable to get %"HOME_DIR_ENV"%");
#else
			perror("Unable to get $"HOME_DIR_ENV"");
#endif
			return false;
		}
		dirlen = strlen(dir);

		s =        ALT_CONFIG_DIR"/fuyunix/";
		n = sizeof(ALT_CONFIG_DIR"/fuyunix/")-1;
	}
	if (dirlen + n >= path_len-1) {
		return -1;
	}
	memcpy(path, dir, dirlen);
	memcpy(path + dirlen, s, n);
	path[dirlen+n] = 0;
	return dirlen+n;
}

int
getSaveDir(char *path, size_t path_len)
{
	char *dir = getenv(SAVE_DIR_ENV);
	size_t dirlen = 0;
	char  *s =        "/fuyunix/";
	size_t n = sizeof("/fuyunix/")-1;
	if (dir) {
		dirlen = strlen(dir);
	} else {
		dir = getenv(HOME_DIR_ENV);
		if (dir == NULL) {
#ifdef _WIN32
			perror("Unable to get %"HOME_DIR_ENV"%");
#else
			perror("Unable to get $"HOME_DIR_ENV"");
#endif
			return false;
		}
		dirlen = strlen(dir);

		s =        ALT_SAVE_DIR"/fuyunix/";
		n = sizeof(ALT_SAVE_DIR"/fuyunix/")-1;
	}
	if (dirlen + n >= path_len-1) {
		return -1;
	}
	memcpy(path, dir, dirlen);
	memcpy(path + dirlen, s, n);
	path[dirlen+n] = 0;
	return dirlen+n;
}

bool
mkdir_all(char *path)
{
	char *s = path;

	while (*s) {
		while (*s && *s != '/')
			s++;
		if (*s)
			s++;
		char c = *s;
		*s = 0;
#ifdef _WIN32
		if (mkdir(path) < 0 && errno != EEXIST) {
#else
		if (mkdir(path, 0755) < 0 && errno != EEXIST) {
#endif
			fprintf(stderr, "failed to make directory %s: %s\n",
					path, strerror(errno));
			*s = c;
			return false;
		}
		*s = c;
		while (*s == '/')
			s++;
	}
	return true;
}

bool
getConfigFile(char *path, size_t path_len)
{
	int n = getConfigDir(path, path_len);
	if (n < 0)
		return false;

	char *s =        "config";
	int   m = sizeof("config");
	if ((size_t)(n + m) >= path_len - 1)
		return false;
	memcpy(path + n, s, m);
	path[n + m] = 0;
	return true;
}

bool
getSaveFile(char *path, size_t path_len, bool create_dir)
{
	int n = getSaveDir(path, path_len);
	if (n < 0)
		return false;

	if (create_dir && !mkdir_all(path)) {
		return false;
	}

	char *s =        "save";
	int   m = sizeof("save");
	if ((size_t)(n + m) >= path_len - 1)
		return false;
	memcpy(path + n, s, m);
	path[n + m] = 0;
	return true;
}

int
readSaveFile(void)
{
	int level = 0;
	char savepath[PATH_MAX];
	FILE *fp;

	if (!getSaveFile(savepath, sizeof(savepath), false)) {
		return level;
	};
	fp = fopen(savepath, "rb");

	if (fp == NULL) {
		if (errno != ENOENT)
			fprintf(stderr, "Couldn't open savefile `%s`: %s\n",
					savepath, strerror(errno));
		return level;
	}

	size_t n = fread(&level, sizeof(int), 1, fp);
	if (n != 1) {
		fprintf(stderr, "Error: %s: only read %zu expected to read %zu\n",
				savepath, n, sizeof(int));
		fclose(fp);
		return level;
	}

	fclose(fp);

	return level;
}

void
writeSaveFile(int level)
{
	char savepath[PATH_MAX];
	FILE *fp;

	if (!getSaveFile(savepath, sizeof(savepath), true)) {
		fprintf(stderr, "Can't write save file: falied to get path to save file\n");
		return;
	}

	fp = fopen(savepath, "wb+");

	if (fp == NULL) {
		fprintf(stderr, "Couldn't open save file `%s`: %s\n",
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

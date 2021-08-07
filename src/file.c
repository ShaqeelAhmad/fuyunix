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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <sys/stat.h>

/*
 * XDG_STATE_HOME is better suited for save files but it's rather new and
 * not everyone knows about it
 */
#define SAVEPATH "XDG_STATE_HOME"

/* Only handles continuous comments. I use it only at the top of file */
static void
handleComments(FILE *fp)
{
	while (fgetc(fp) == '#') {
		while (fgetc(fp) != '\n');
	}
}

static char *
getPath(char *xdgdir, char *file, char *altdir)
{
	/* FIXME The variable naming sucks */
	char *path;
	char *fullpath;
	char *dir = "/fuyunix";

	path = getenv(xdgdir);
	/* TODO Improve path handling */
	if (path == NULL) {
		path = getenv("HOME");
		if (path == NULL) {
			fprintf(stderr, "HOME is not set\n");
			return NULL;
		}

		dir = altdir;
	}

	char *dirpath;
	size_t size = strlen(path) + strlen(dir);

	dirpath = malloc(size);
	if (dirpath == NULL) {
		fputs("Unable to allocate memory for path\n", stderr);
		return NULL;
	}
	snprintf(dirpath, size + 1, "%s%s", path, dir);

	if (mkdir(dirpath, 0755) != 0)
		if(errno != EEXIST) {
			perror("Unable to create save directory\n");
			free(dirpath);
			return NULL;
		}

	size = size + strlen(file);
	fullpath = malloc(size);
	if (fullpath == NULL) {
		fputs("Unable to allocate memory for path\n", stderr);
		free(dirpath);
		return NULL;
	}

	snprintf(fullpath, size + 1, "%s%s", dirpath, file);

	free(dirpath);

	return fullpath;
}

/* Only reads level number for now */
int
readSaveFile(void)
{
	int level;
	char *savepath;
	FILE *fp;

	savepath  = getPath(SAVEPATH, "/save", "/.local/state/fuyunix");

	fp = fopen(savepath, "r");

	if (fp == NULL) {
		free(savepath);
		/* Default level = 1 */
		return 1;
	}

	handleComments(fp);

	fscanf(fp, "%*s%d", &level);

	fclose(fp);

	free(savepath);

	return level;
}

/*
 * Note: If you're writing a "real" game, you probably want to encrypt the
 * save files in some way to avoid people cheating in the game
 */
void
writeSaveFile(int level)
{
	char *comment = "# This file should not be edited manually\n"
					"# Also I only handle comments on the top of file";
	char *savepath;
	FILE *fp;

	savepath  = getPath(SAVEPATH, "/save", "/.local/state/fuyunix");

	fp = fopen(savepath, "w+");
	if (fp == NULL) {
		fprintf(stderr, "Couldn't open savefile %s\n", savepath);
		free(savepath);
		return;
	}

	fprintf(fp, "%s\n", comment);

	fprintf(fp, "Level %d\n", level);

	fclose(fp);

	free(savepath);
}

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

/* XDG_STATE_HOME is better suited for save files but it's rather new and
 * not everyone knows about it */
#define XDG_STATE_HOME "XDG_STATE_HOME"
#define XDG_CONFIG_HOME "XDG_CONFIG_HOME"

static int level = 0;

void
handleComments(FILE *fp)
{
	while (fgetc(fp) == '#') {
		while (fgetc(fp) != '\n');
	}
}

char *
getPath(char *xdgdir, char *file, char *altdir)
{
	char *path;
	char *str;
	char *dir = "/fuyunix";

	path = getenv(xdgdir);
	if (path == NULL) {
		path = getenv("HOME");
		if (path == NULL)
			fprintf(stderr, "HOME is not set\n");

		dir = altdir;
	}

	char *dirpath;
	size_t size = strlen(path) + strlen(dir);

	dirpath = malloc(size);
	snprintf(dirpath, size + 1, "%s%s", path, dir);

	if (mkdir(dirpath, 0755) != 0)
		if(errno != EEXIST)
			perror("Unable to create save directory\n");

	size = size + strlen(file);
	str = malloc(size);

	snprintf(str, size + 1, "%s%s", dirpath, file);

	free(dirpath);

	return str;
}

void
readSaveFile(void)
{
	char *savepath;
	FILE *fp;

	savepath  = getPath(XDG_STATE_HOME, "/save", "/.local/state/fuyunix");

	fp = fopen(savepath, "r");

	if (fp == NULL) {
		free(savepath);
		return;
	}

	handleComments(fp);

	fscanf(fp, "%*s%d", &level);

	fclose(fp);

	free(savepath);
}

/* Note: If you're writing a "real" game, you probably want to encrypt the
 * save files in some way to avoid people cheating in the game */
void
writeSaveFile(void)
{
	char *savepath;
	FILE *fp;

	savepath  = getPath(XDG_STATE_HOME, "/save", "/.local/state/fuyunix");

	fp = fopen(savepath, "w+");
	if (fp == NULL) {
		perror("Couldn't open savefile\n");
		free(savepath);
		return;
	}

	fprintf(fp, "# This file should not be edited manually\n"
			"# Also  comments only work on the top of file\n");

	fprintf(fp, "Level %d\n", level);

	fclose(fp);

	free(savepath);
}

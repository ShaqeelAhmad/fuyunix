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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

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
getPath(char *xdg, char *file)
{
	char *dirpath;
	char *dir = "/fuyunix";
	if ((dirpath = getenv(xdg)) == NULL) {
		if ((dirpath = getenv("HOME")) == NULL) {
			perror("Unable to get HOME");
			exit(1);
		}
		dir = "/.fuyunix";
	}
	char *fullpath = calloc(strlen(dir) + strlen(dirpath) + strlen(file) + 1,
			sizeof(char));

	strcpy(fullpath, dirpath);
	strcat(fullpath, dir);

	if (mkdir(fullpath, 0755) < 0 && errno != EEXIST)
		perror("Unable to create directory");;

	strcat(fullpath, file);

	return fullpath;
}

/* Only reads level number for now */
int
readSaveFile(void)
{
	int level;
	char *savepath;
	FILE *fp;

	savepath  = getPath(SAVEPATH, "/save");

	fp = fopen(savepath, "r");

	/* Ignore file not existing for reading and use default values */
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

	savepath  = getPath(SAVEPATH, "/save");

	fp = fopen(savepath, "w+");
	if (fp == NULL) {
		perror("Couldn't open savefile");
		free(savepath);
		return;
	}

	fprintf(fp, "%s\n", comment);

	fprintf(fp, "Level %d\n", level);

	fclose(fp);

	free(savepath);
}

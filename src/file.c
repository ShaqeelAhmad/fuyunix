#include <stdlib.h>
#include <stdio.h>

static int level = 0;

static char *xdgstatehome;
static char *xdgconfighome;

char *
getSavePath(void)
{
	char *path;
	char *str;
	char *dir = "/fuyunix";
	char *file = "/save";

	/* XDG_STATE_HOME is better suited for save files but it's rather new and
	 * not everyone knows about it */
	path = getenv("XDG_STATE_HOME");
	if (path == NULL) {
		path = getenv("HOME");
		if (path == NULL)
			fprintf(stderr, "HOME is not set\n");

		dir = "/.fuyunix";
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
handleComments(FILE *fp)
{
	while (fgetc(fp) == '#') {
		while (fgetc(fp) != '\n');
	}
}

void
readSaveFile(void)
{
	char *savepath;
	FILE *fp;

	savepath  = getSavePath();

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

	savepath  = getSavePath();

	fp = fopen(savepath, "w+");
	if (fp == NULL) {
		perror("Couldn't open savefile\n");
		free(savepath);
		return;
	}

	fprintf(fp, "# This file should not be edited by the user\n"
			"# Do not change the order of the variables"
			"(Only the values are parsed)\n"
			"# Comments only work on the top of file\n");

	fprintf(fp, "Level %d\n", level);

	fclose(fp);

	free(savepath);
}

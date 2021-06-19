#define _POSIX_C_SOURCE 2
#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <SDL2/SDL.h>
#include <unistd.h>

#include "drw.h"
#include "keys.h"
#include "fuyunix.h"

/* global variables */
static bool quit = false;
static int level = 0;

/* Function definitions */
char *
getSavePath(void)
{
	char *path;
	char *str;
	char *dir = "/fuyunix";
	char *file = "/save";

	path = getenv("XDG_DATA_HOME");
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

/* Other parts of program will call quitloop to exit the game */
void
quitloop(void)
{
	quit = true;
}

void
run(void)
{
	readSaveFile();

	SDL_Event event;

	while (!quit) {
		while (SDL_PollEvent(&event) != 0) {
			if (event.type == SDL_QUIT)
				quitloop();
			else if (event.type == SDL_KEYDOWN)
				handleKeys(&event.key);
		}
		drw();
	}

	writeSaveFile();
}

int
main(int argc, char *argv[])
{
	int x;

	while ((x = getopt(argc, argv, "v")) != -1) {
		if (x == 'v') {
			puts("Fuyunix: "VERSION);
			return EXIT_SUCCESS;
		} else {
			fputs("Usage: fuyunix [-v]\n"
					"Run fuyunix without any arguments to start the game\n",
					stderr);
			return EXIT_FAILURE;
		}
	}

	init();

	run();

	cleanup();

	return EXIT_SUCCESS;
}

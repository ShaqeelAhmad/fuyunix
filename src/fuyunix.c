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

/* I don't think this is how I should do this */
#define _POSIX_C_SOURCE 2

#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <SDL.h>
#include <unistd.h>

#include "drw.h"
#include "keys.h"
#include "fuyunix.h"

/* global variables */
static bool quit = false;

/* Function definitions */

void
quitloop()
{
	quit = true;
}

static void
run(void)
{
	homeMenu();

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
}

int
main(int argc, char *argv[])
{
	int x;

	if (argc > 1) {
		while ((x = getopt(argc, argv, "v")) != -1) {
			if (x == 'v') {
				puts(NAME": "VERSION);
				return EXIT_SUCCESS;
			}
		}
		fputs("Usage: fuyunix [-v]\n"
				"Run fuyunix without any arguments to start the game\n",
				stderr);
		return EXIT_FAILURE;
	}

	init();

	run();

	cleanup();

	return EXIT_SUCCESS;
}

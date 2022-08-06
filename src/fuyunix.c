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

#include <SDL.h>
#include <stdbool.h>
#include <stdio.h>
#include <unistd.h>

#include "drw.h"
#include "fuyunix.h"
#include "keys.h"

static bool quit = false;

void
quitloop()
{
	quit = true;
}

static void
run(void)
{
	SDL_Event event;
	while (!quit) {
		handleKeys();

		while (SDL_PollEvent(&event) != 0) {
			if (event.type == SDL_QUIT)
				quitloop();
			else if (event.type == SDL_KEYDOWN)
				menuHandleKeys(event.key.keysym.scancode);
		}

		drw();
	}
}

int
main(int argc, char *argv[])
{
	int x;
	int flags = SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE;

	if (argc > 1) {
		while ((x = getopt(argc, argv, "vlf")) != -1) {
			switch (x) {
			case 'v':
				puts(NAME": "VERSION);
				return EXIT_SUCCESS;
			case 'l':
				listFunc();
				return EXIT_SUCCESS;
			case 'f':
				flags |= SDL_WINDOW_FULLSCREEN;
				break;
			default:
				fputs("Usage: fuyunix [-v|-l|-f]\n", stderr);
				return EXIT_FAILURE;
			}
		}
	}

	init(flags);

	run();

	cleanup();

	return EXIT_SUCCESS;
}

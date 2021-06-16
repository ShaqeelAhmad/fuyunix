#define _POSIX_C_SOURCE 2
#include <stdio.h>
#include <stdlib.h>
#include <SDL2/SDL.h>
#include <stdbool.h>
#include <unistd.h>

#include "keys.h"
#include "fuyunix.h"
#include "drw.h"

/* structs */

/* Function declarations */
static bool quit = false;

/* Function definitions */

/* Other parts of program will call quitloop to exit the game */
void
quitloop(void)
{
	quit = true;
}

void
run(void)
{
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

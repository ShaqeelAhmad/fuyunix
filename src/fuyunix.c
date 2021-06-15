#include <stdio.h>
#include <stdlib.h>
#include <SDL2/SDL.h>
#include <stdbool.h>

#include "keys.h"
#include "fuyunix.h"
#include "drw.h"

/* structs */

/* Function declarations */

/* Function definitions */
void 
run(void)
{
	SDL_Event event;
	bool quit = false;

	while (!quit) {
		while (SDL_PollEvent(&event) != 0) {
			if (event.type == SDL_QUIT)
				quit = true;
			else if (event.type == SDL_KEYDOWN)
				quit = handleKeys(&event.key);

		}
		drw();
	}

}

int
main(int argc, char *argv[])
{
	int i = 0;
	while (argc > 1 && i < argc) {
		if (strcmp(argv[i], "-v") == 0) {
			puts(VERSION);
			return EXIT_SUCCESS;
		}
		i++;
	}
	if (argc > 1) {
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

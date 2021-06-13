#include <stdio.h>
#include <stdlib.h>
#include <SDL2/SDL.h>
#include <stdbool.h>

#include "fuyunix.h"

/* structs */

/* Function declarations */

/* Function definitions */
void
init(void)
{
	SDL_Init(SDL_INIT_VIDEO);

	SDL_GetDesktopDisplayMode(0, &game.dm);

	game.win = SDL_CreateWindow(GAMENAME,
			SDL_WINDOWPOS_UNDEFINED,
			SDL_WINDOWPOS_UNDEFINED,
			game.dm.w,
			game.dm.h,
			0);

	game.rnd = SDL_CreateRenderer(game.win, -1, 0);
}

void 
cleanup(void)
{
	SDL_DestroyRenderer(game.rnd);
	SDL_DestroyWindow(game.win);

	SDL_Quit();
}

int
main(int argc, char *argv[])
{
	int i;
	if (argc > 1) {
		for (i = 1; i < argc; i++) {
			if (strcmp(argv[i], "-v") == 0)
				puts(VERSION);
		}
		fputs("Usage fuyunix [-v]\n"
				"Run fuyunix without any arguments to start the game\n",
				stderr);
		return EXIT_FAILURE;
	}

	init();
	fprintf(stderr, "%s\n", resourcepath);

	/* run(); */

	cleanup();

	return EXIT_SUCCESS;
}

#include <stdio.h>
#include <stdlib.h>
#include <SDL2/SDL.h>
#include <stdbool.h>

#include "fuyunix.h"
#include "drw.h"

/* structs */

/* Function declarations */

/* Function definitions */
int
main(int argc, char *argv[])
{
	int i;
	if (argc > 1) {
		for (i = 1; i < argc; i++) {
			if (strcmp(argv[i], "-v") == 0) {
				puts(VERSION);
				return EXIT_SUCCESS;
			}
		}
		fputs("Usage: fuyunix [-v]\n"
				"Run fuyunix without any arguments to start the game\n",
				stderr);
		return EXIT_FAILURE;
	}

	init();

	/* run(); */

	cleanup();

	return EXIT_SUCCESS;
}

#include <stdio.h>
#include <SDL2/SDL.h>
#include "fuyunix.h"

void
drwmenu(void)
{
	fprintf(stderr, "%d\n", game.dm.w);
}

void 
drw(void)
{
	fprintf(stderr, "%s\n", resourcepath);

}

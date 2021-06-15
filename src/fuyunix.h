#ifndef _FUYUNIX_H_
#define _FUYUNIX_H_

#define VERSION "1.01"
#define GAMENAME "fuyunix"

static const char resourcepath[] = "/usr/local/games/fuyunix/";

struct Game {
	SDL_Window *win;
	SDL_Renderer *rnd;

	SDL_Surface *surf;

	SDL_DisplayMode dm;

	int h;
	int w;
};

#endif

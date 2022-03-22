PREFIX = /usr/local
MANDIR = $(PREFIX)/share/man/man6
DATADIR = $(PREFIX)/games
BINDIR = $(PREFIX)/bin
RESOURCE_PATH = $(DATADIR)

CC = cc

CFLAGS = -g -Wall -Werror -pedantic -Wextra -std=c99 -Wno-unused-parameter

CFLAGS += -DRESOURCE_PATH=\"$(RESOURCE_PATH)\" `pkg-config --cflags cairo sdl2`

## Disables cpu specific vector operations which doesn't work on certain
## compilers. You might want to remove this if your compiler supports it.
CFLAGS += -DSDL_DISABLE_IMMINTRIN_H

LDFLAGS = `pkg-config --libs sdl2 cairo SDL2_image` -lm

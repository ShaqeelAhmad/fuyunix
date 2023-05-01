PREFIX = /usr/local
MANDIR = $(PREFIX)/share/man/man6
DATADIR = $(PREFIX)/games
BINDIR = $(PREFIX)/bin
GAME_DATA_DIR = $(DATADIR)/fuyunix/data/

CC = cc

CFLAGS = -g -Wall -pedantic -Wextra -std=c99 -Wno-unused-parameter

VERSION = `git log -1 --format=dev-%h`

CFLAGS += -D_POSIX_C_SOURCE=200809L \
		  -DDATA_DIR=\"$(DATADIR)\" \
		  -DVERSION=\"$(VERSION)\" \
		  -DGAME_DATA_DIR=\"$(GAME_DATA_DIR)\" \
		  `pkg-config --cflags sdl2`

## Disable cpu specific vector operations which doesn't work on certain
## compilers.
CFLAGS += -DSDL_DISABLE_IMMINTRIN_H

LDFLAGS = `pkg-config --libs SDL2_ttf SDL2_image sdl2` -lm

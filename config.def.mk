PREFIX = /usr/local
MANDIR = $(PREFIX)/share/man/man6
DATADIR = $(PREFIX)/games
BINDIR = $(PREFIX)/bin
RESOURCE_PATH = $(DATADIR)
LEVELS_DIR=$(DATADIR)/fuyunix/data/levels/

CC = cc

CFLAGS = -g -Wall -Werror -pedantic -Wextra -std=c99 -Wno-unused-parameter

VERSION = `git log -1 --format=dev-%h`

CFLAGS += -D_POSIX_C_SOURCE=200809L \
		  -DRESOURCE_PATH=\"$(RESOURCE_PATH)\" \
		  -DLEVELS_DIR=\"$(LEVELS_DIR)\" \
		  -DVERSION=\"$(VERSION)\" \
		  `pkg-config --cflags cairo sdl2`

## Disable cpu specific vector operations which doesn't work on certain
## compilers.
CFLAGS += -DSDL_DISABLE_IMMINTRIN_H

LDFLAGS = `pkg-config --libs cairo SDL2_image sdl2` -lm

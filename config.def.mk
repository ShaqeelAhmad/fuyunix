PREFIX = /usr/local
MANDIR = $(PREFIX)/share/man/man6
DATADIR = $(PREFIX)/games
BINDIR = $(PREFIX)/bin
GAME_DATA_DIR = $(DATADIR)/fuyunix/data/

CC = cc

TARGET = sdl
# TARGET = wayland

VERSION = `git log -1 --format=dev-%h`

CFLAGS = -g -Wall -pedantic -Wextra -std=c11 -Wno-unused-parameter

CFLAGS += -D_POSIX_C_SOURCE=200809L \
		  -DVERSION=\"$(VERSION)\" \
		  -DGAME_DATA_DIR=\"$(GAME_DATA_DIR)\"

CFLAGS_sdl = `pkg-config --cflags SDL2_ttf SDL2_image sdl2`
LDFLAGS_sdl = `pkg-config --libs SDL2_ttf SDL2_image sdl2` -lm

## Disable cpu specific vector operations which doesn't work on certain
## compilers.
CFLAGS_sdl += -DSDL_DISABLE_IMMINTRIN_H


CFLAGS_wayland = `pkg-config --cflags wayland-client xkbcommon`
LDFLAGS_wayland = `pkg-config --libs wayland-client xkbcommon` -lm

CFLAGS += $(CFLAGS_$(TARGET))
LDFLAGS += $(LDFLAGS_$(TARGET))

DIR = /usr/local
MANDIR = $(DIR)/man/man6

DATADIR = $(DIR)/games/fuyunix

RESOURCE_PATH = $(DATADIR)

CC = cc

CFLAGS = -g -Wall -Werror -pedantic -Wextra -std=c99 -Wno-unused-parameter \
	-DRESOURCE_PATH=\"$(RESOURCE_PATH)\"

CFLAGS += `pkg-config --cflags cairo sdl2`

## Required for tcc
CFLAGS += -DSDL_DISABLE_IMMINTRIN_H

LDFLAGS = `pkg-config --libs sdl2 cairo SDL2_image`
LDFLAGS +=  -lm

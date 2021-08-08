# CC = gcc
# CC = clang
DIR = /usr/local

CFLAGS = $(shell sdl2-config --cflags)
CFLAGS += -Wall -Werror -pedantic -Wextra -std=c99 -Wno-unused-parameter

CFLAGS += $(shell pkg-config --cflags cairo)

# Required for tcc
CFLAGS += -DSDL_DISABLE_IMMINTRIN_H -DSDL_MAIN_HANDLED

## Doesn't work with tcc
# LDFLAGS = $(shell sdl2-config --libs) -lSDL2_image
LDFLAGS = -lSDL2 -lSDL2_image -lcairo

SRC = fuyunix.c keys.c drw.c file.c
OBJ = $(SRC:.c=.o)

all: fuyunix

.c.o:
	$(CC) -c $< $(CFLAGS)

fuyunix: $(OBJ)
	$(CC) -o $@ $(OBJ) $(LDFLAGS) $(CFLAGS)

clean:
	$(RM) fuyunix $(OBJ)

install: all
	mkdir -p $(DIR)/bin
	cp -f fuyunix $(DIR)/bin
	chmod 755 $(DIR)/bin/fuyunix

uninstall:
	$(RM) $(DIR)/bin/fuyunix

.PHONY: clean install uninstall

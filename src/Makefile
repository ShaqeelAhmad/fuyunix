# CC = gcc
# CC = clang
DIR = /usr/local

# CFLAGS = $(shell pkg-config --cflags cairo) $(shell sdl2-config --cflags)
## I'm not sure why but bmake doesn't work with the syntax above

CFLAGS = `sdl2-config --cflags` `pkg-config --cflags cairo`

CFLAGS += -Wall -Werror -pedantic -Wextra -std=c99 -Wno-unused-parameter

# Required for tcc
CFLAGS += -DSDL_DISABLE_IMMINTRIN_H -DSDL_MAIN_HANDLED

LDFLAGS = -lSDL2 -lSDL2_image -lcairo

SRC = fuyunix.c keys.c drw.c file.c
OBJ = $(SRC:.c=.o)

all: fuyunix

.c.o:
	$(CC) -c $< $(CFLAGS)

fuyunix: $(OBJ)
	$(CC) -o $@ $(OBJ) $(LDFLAGS)

clean:
	rm -f fuyunix $(OBJ)

install: all
	mkdir -p $(DIR)/bin
	cp -f fuyunix $(DIR)/bin
	chmod 755 $(DIR)/bin/fuyunix

uninstall:
	rm -f $(DIR)/bin/fuyunix


.PHONY: clean install uninstall

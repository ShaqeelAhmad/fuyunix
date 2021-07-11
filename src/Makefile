CC = gcc
SRC = fuyunix.c keys.c drw.c file.c
OBJ = $(SRC:.c=.o)

DIR = /usr/local

CFLAGS = $(shell sdl2-config --cflags)
LDFLAGS = $(shell sdl2-config --libs)

CFLAGS += -Wall -Werror -pedantic -Wextra -std=c99 -Wno-unused-parameter
LDFLAGS += -lSDL2_image

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

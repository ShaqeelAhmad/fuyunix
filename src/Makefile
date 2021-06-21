CC = gcc
SRC = fuyunix.c keys.c drw.c
OBJ = $(SRC:.c=.o)

DIR = /usr/local

CFLAGS = -Wall -Werror -pedantic -Wextra -std=c99
LDFLAGS = `sdl2-config --cflags --libs` -lSDL2_image

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

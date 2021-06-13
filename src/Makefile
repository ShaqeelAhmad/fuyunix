CC = gcc
SRC = fuyunix.c keys.c drw.c
OBJ = ${SRC:.c=.o}

CFLAGS = -Wall -pedantic -Wextra -std=c99
LDFLAGS = `sdl2-config --cflags --libs` -lSDL2_image

all: fuyunix

.c.o:
	${CC} -c ${CFLAGS} $<

fuyunix: ${OBJ}
	${CC} -o $@ ${OBJ} ${LDFLAGS} ${CFLAGS}

clean:
	rm -f fuyunix ${OBJ}

install:
	mkdir -p ${DESTDIR}${PREFIX}/bin
	cp -f fuyunix ${DESTDIR}${PREFIX}/bin
	chmod 755 ${DESTDIR}${PREFIX}/bin/fuyunix
	mkdir -p ${DESTDIR}${MANPREFIX}/man6
	cp -f fuyunix.6 ${DESTDIR}${MANPREFIX}/man6/fuyunix.6
	chmod 644 ${DESTDIR}${MANPREFIX}/man6/fuyunix.6

uninstall:
	rm -f ${DESTDIR}${PREFIX}/bin/fuyunix
	rm -f ${DESTDIR}${MANPREFIX}/man6/fuyunix.6

.PHONY: clean

.POSIX:
include ../config.mk

VERSION = `git log -1 --format=dev-%h`
CFLAGS += -DVERSION=\"$(VERSION)\"

SRC = fuyunix.c keys.c drw.c util.c scfg.c
OBJ = $(SRC:.c=.o)

all: fuyunix

.c.o:
	$(CC) $(CFLAGS) -c $<

fuyunix: $(OBJ)
	$(CC) -o $@ $(OBJ) $(LDFLAGS)

clean:
	rm -f fuyunix $(OBJ)

install: all
	mkdir -p $(BINDIR)
	cp -f fuyunix $(BINDIR)
	chmod 755 $(BINDIR)/fuyunix

uninstall:
	rm -f $(BINDIR)/fuyunix


.PHONY: clean install uninstall

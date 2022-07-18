.POSIX:

include ../config.mk

VERSION = `git log -1 --format=%h`
CFLAGS += -DVERSION=\"dev-$(VERSION)\"

SRC = fuyunix.c keys.c drw.c file.c alloc.c
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

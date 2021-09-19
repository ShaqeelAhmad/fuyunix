include ../config.mk

SRC = fuyunix.c keys.c drw.c file.c alloc.c
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

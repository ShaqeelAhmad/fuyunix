DIR = /usr/local/man

all:
	@${MAKE} -C src

clean:
	@${MAKE} clean -C src

install:
	@${MAKE} install -C src
	mkdir -p $(DIR)/man6
	cp -f man/fuyunix.6 $(DIR)/man6/fuyunix.6
	chmod 644 $(DIR)/man6/fuyunix.6

uninstall:
	@${MAKE} uninstall -C src
	rm -f ${DESTDIR}${MANPREFIX}/man6/fuyunix.6

.PHONY: clean install uninstall

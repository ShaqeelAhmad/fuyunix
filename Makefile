.POSIX:
include config.mk

all:
	$(MAKE) -C src

clean:
	$(MAKE) clean -C src
	rm -f fuyunix.6

man:
	scdoc < fuyunix.6.scd > fuyunix.6

install: man
	$(MAKE) install -C src
	mkdir -p $(MANDIR)
	cp -f fuyunix.6 $(MANDIR)/fuyunix.6
	chmod 644 $(MANDIR)/fuyunix.6
	mkdir -p $(DATADIR)/fuyunix/
	cp -rf data/ $(DATADIR)/fuyunix/

uninstall:
	$(MAKE) uninstall -C src
	rm -f $(MANDIR)/fuyunix.6
	rm -rf $(DATADIR)/fuyunix

.PHONY: clean install uninstall

include config.mk

all:
	$(MAKE) -C src

clean:
	$(MAKE) clean -C src

install:
	$(MAKE) install -C src
	mkdir -p $(MANDIR)
	cp -f man/fuyunix.6 $(MANDIR)/fuyunix.6
	chmod 644 $(MANDIR)/fuyunix.6
	mkdir -p $(DATADIR)
	cp -rf data/ $(DATADIR)/

uninstall:
	$(MAKE) uninstall -C src
	rm -f $(MANDIR)/fuyunix.6
	rm -rf $(DATADIR)

.PHONY: clean install uninstall

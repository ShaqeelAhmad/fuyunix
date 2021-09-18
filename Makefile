include config.mk

all:
	@$(MAKE) -C src

clean:
	@$(MAKE) clean -C src

install:
	@$(MAKE) install -C src
	@mkdir -p $(MANDIR)
	@cp -f man/fuyunix.6 $(MANDIR)/fuyunix.6
	@chmod 644 $(MANDIR)/fuyunix.6
	@mkdir -p $(DATADIR)
	@cp -r data/ $(DATADIR)/

uninstall:
	@$(MAKE) uninstall -C src
	@rm -f $(DIR)/fuyunix.6

.PHONY: clean install uninstall

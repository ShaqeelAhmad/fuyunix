DIR = /usr/local/man/man6

all:
	@$(MAKE) -C src

clean:
	@$(MAKE) clean -C src

install:
	@$(MAKE) install -C src
	mkdir -p $(DIR)
	cp -f man/fuyunix.6 $(DIR)/fuyunix.6
	chmod 644 $(DIR)/fuyunix.6

uninstall:
	@$(MAKE) uninstall -C src
	$(RM) $(DIR)/fuyunix.6

.PHONY: clean install uninstall

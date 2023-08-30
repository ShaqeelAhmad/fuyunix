.POSIX:
include config.mk

WL_PROTOCOLS_DIR = /usr/share/wayland-protocols/
WL_SCANNER = wayland-scanner
WL_SRC = xdg-shell-protocol.c xdg-decoration-unstable-protocol.c
WL_HDR = xdg-shell-client-protocol.h xdg-decoration-unstable-client-protocol.h
XDG_SHELL = $(WL_PROTOCOLS_DIR)/stable/xdg-shell/xdg-shell.xml
XDG_DECORATION = $(WL_PROTOCOLS_DIR)/unstable/xdg-decoration/xdg-decoration-unstable-v1.xml


all: fuyunix man

clean:
	rm -f fuyunix.6
	rm -f fuyunix
	rm -f $(WL_SRC) $(WL_HDR)

man: fuyunix.6

fuyunix.6:
	scdoc < fuyunix.6.scd > fuyunix.6

install: all
	mkdir -p $(BINDIR)
	cp -f fuyunix $(BINDIR)
	chmod 755 $(BINDIR)/fuyunix
	mkdir -p $(MANDIR)
	cp -f fuyunix.6 $(MANDIR)/fuyunix.6
	chmod 644 $(MANDIR)/fuyunix.6
	mkdir -p $(DATADIR)/fuyunix/
	cp -rf data/ $(DATADIR)/fuyunix/

uninstall:
	rm -f $(BINDIR)/fuyunix
	rm -f $(MANDIR)/fuyunix.6
	rm -rf $(DATADIR)/fuyunix

sdl:

wayland: $(WL_HDR) $(WL_SRC)

xdg-shell-protocol.c:
	$(WL_SCANNER) private-code $(XDG_SHELL) $@

xdg-shell-client-protocol.h:
	$(WL_SCANNER) client-header $(XDG_SHELL) $@

xdg-decoration-unstable-protocol.c:
	$(WL_SCANNER) private-code $(XDG_DECORATION) $@

xdg-decoration-unstable-client-protocol.h:
	$(WL_SCANNER) client-header $(XDG_DECORATION) $@

fuyunix: src/*.c $(TARGET)
	$(CC) unity_$(TARGET).c -o $@ $(CFLAGS) $(LDFLAGS)

.PHONY: clean install uninstall

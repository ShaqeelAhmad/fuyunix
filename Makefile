include config.mk

CFLAGS_sdl = `pkg-config --cflags SDL2_ttf SDL2_image sdl2`
LDFLAGS_sdl = `pkg-config --libs SDL2_ttf SDL2_image sdl2` -lm

CFLAGS_wayland = `pkg-config --cflags wayland-client wayland-cursor xkbcommon cairo freetype2`
LDFLAGS_wayland = `pkg-config --libs  wayland-client wayland-cursor xkbcommon cairo freetype2` -lm

CFLAGS += $(CFLAGS_$(TARGET))
LDFLAGS += $(LDFLAGS_$(TARGET))

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

install-fuyunix: fuyunix
	mkdir -p $(BINDIR)
	cp -f fuyunix $(BINDIR)
	chmod 755 $(BINDIR)/fuyunix
	mkdir -p $(DATADIR)/fuyunix/
	cp -rf data/ $(DATADIR)/fuyunix/

install-man: man
	mkdir -p $(MANDIR)
	cp -f fuyunix.6 $(MANDIR)/fuyunix.6
	chmod 644 $(MANDIR)/fuyunix.6

install: install-fuyunix install-man

uninstall:
	rm -f $(BINDIR)/fuyunix
	rm -rf $(DATADIR)/fuyunix
	rm -f $(MANDIR)/fuyunix.6

xdg-shell-protocol.c:
	$(WL_SCANNER) private-code $(XDG_SHELL) $@

xdg-shell-client-protocol.h:
	$(WL_SCANNER) client-header $(XDG_SHELL) $@

xdg-decoration-unstable-protocol.c:
	$(WL_SCANNER) private-code $(XDG_DECORATION) $@

xdg-decoration-unstable-client-protocol.h:
	$(WL_SCANNER) client-header $(XDG_DECORATION) $@

unity_sdl.c:

unity_wayland.c: $(WL_HDR) $(WL_SRC)

fuyunix: src/*.c unity_$(TARGET).c
	$(CC) unity_$(TARGET).c -o $@ $(CFLAGS) $(LDFLAGS)

.PHONY: clean install uninstall

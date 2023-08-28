.POSIX:
include config.mk

all: fuyunix man

clean:
	rm -f fuyunix.6
	rm -f fuyunix

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

fuyunix: src/*.c
	$(CC) unity_$(TARGET).c -o $@ $(CFLAGS) $(LDFLAGS)

.PHONY: clean install uninstall

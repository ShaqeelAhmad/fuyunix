# Fuyunix

**NOTE: this game is still under early stages of development**

A platformer game.

## Dependencies

- c11 C compiler
- make
- cairo
- freetype2
- scdoc (optional, for manpages)

- sdl2 >= 2.0.22

Alternative dependencies, if you want to use the native wayland instead of SDL
(set TARGET to wayland in config.mk):

- libwayland
- xkbcommon

## Portablility

I use Linux and can't guarantee that the program will work on other operating
systems

## TODO

* Joystick / gamepad support.

## Screenshots

TBD

## Installation

```
$ cp config.def.mk config.mk
$ make
# make install
```

Alternatively, if you didn't install `scdoc` or don't want manpages.

```
$ cp config.def.mk config.mk
$ make fuyunix
# make install-fuyunix
```

Modify `config.mk` for configurations.

Note that you could change `GAME_DATA_DIR` in `config.mk` to `./data/` to play
the game in this directory without installing.

## Usage

Look at the manpage for options.

Don't pass any arguments to start the game.

## License
CC0 for the images

[GPLv3 LICENSE](LICENSE) for the source code
Also GPLv3 for the fonts in [data/fonts](data/fonts) which
are taken from https://www.gnu.org/software/freefont/

[MIT LICENSE.scfg](src/LICENSE.scfg) for the scfg source code (the files
src/scfg.c and src/scfg.h) which is taken from
https://git.sr.ht/~emersion/libscfg

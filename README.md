# Fuyunix

**NOTE: this game is still under early stages of development**

A small platformer game using SDL2

## Dependencies
- c11 C compiler*
- sdl2 >= 2.0.22
- sdl2_image
- sdl2_ttf
- make
- scdoc (for manpages)

\* Only tcc, gcc and clang are tested.
Other compilers might not work.

## Portablility

I use Linux and can't guarantee that the program will work on other operating
systems

## TODO

* Joystick / gamepad support.

## Screenshots

TBD

## Files
`src/` directory contains the source code

`man/` contains the man page

`data/` contains the assets

## Installation
```
$ cp config.def.mk config.mk
$ make
# make install
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

[MIT LICENSE.scfg](src/LICENSE.scfg) for the scfg source code (specifically
src/scfg.c and src/scfg.h) which is taken from
https://git.sr.ht/~emersion/libscfg

# Fuyunix

A small platformer game using SDL2

Note: This game is still under development.

## Dependencies
- c99 C compiler*
- cairo
- sdl2 >= 2.0.22
- sdl2_image
- make
- scdoc (for manpages)

\* Only tcc, gcc and clang are tested.
Other compilers might not work.

## Portablility
I use Linux and can't guarantee that the program will work on other operating
systems

## Goals
* Small and works on low end / old hardware.
* Minimal dependencies

## TODO
* Joystick / gamepad support.
* Reading levels from a file.
* Having special textures for stuff

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

## Usage

Look at the manpage for options.

Don't pass any arguments to start the game.

## License
CC0 for the images

[GPLv3 LICENSE](LICENSE) for the source code

[MIT LICENSE.scfg](src/LICENSE.scfg) for the scfg source code (specifically
src/scfg.c and src/scfg.h) which is taken from
https://git.sr.ht/~emersion/libscfg

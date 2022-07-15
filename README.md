# Fuyunix
A small platformer game using SDL2

## Dependencies
- c99 C compiler*
- cairo
- sdl2
- sdl2_image
- make

\* Only tcc, gcc and clang are tested.
Other compilers might not work.

## Portablility
I use Linux and can't guarantee that the program will work on other operating
systems

## Goals
* Lightweight (so my potato pc can handle it)
* Minimal dependencies
* Joystick / gamepad support.

## Screenshots
TBD

## Non-goals
* Mouse support.

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
`-v` for version

`-l` for list of functions

don't pass any arguments to start the game.

## License
GPLv3 [LICENSE](LICENSE) for the source code and CC0 for the images

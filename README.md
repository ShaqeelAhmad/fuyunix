# fuyunix
A small platformer game using SDL2

# Dependencies
```
c compiler*
cairo
sdl2
sdl2_image
make
```
\* Only tcc, gcc and clang are tested.
Other compilers might not work.

# Portablility
I use Linux and can't guarantee that the program will work on other operating
systems

# Goals
* Should be less than 100 MB ignoring git files and including all the assets
  (I doubt it'll go that far)
* Minimal dependencies
* Joystick / gamepad support.

# Non-goals
* Mouse support.

# Files
`src/` directory contains the source code

`man/` contains the man page

`data/` contains the assets

# Installation
```sh
make
make install
```
edit `config.mk` for any configurations

# Usage
`-v` for version

`-l` for list of functions

don't pass any arguments to start the game.

# License
GPLv3 [LICENSE](LICENSE) for the source code and CC0 for the images

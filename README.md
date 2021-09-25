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
And I haven't been able to get cproc to compile sdl programs.

# Portablility
I use Linux and can't guarantee that the program will work on other operating
systems

# Contributing
I have no idea how to handle contributions, so don't.

# Non-goals
* Joystick / gamepad support. I don't have one to try it out.
* Mouse support. I don't care about it.

# Files
`src/` directory contains the source code
`man/` contains the man page
`data/` contains the assets

# Installation
```sh
make
make install
```
use `make CC=cc` to specify a compiler or just edit the config.mk.

replace cc with the compiler of your choice

# Usage
-v for version
-l for list of functions

don't pass any arguments to start the game.

# License
GPLv3 [LICENSE](LICENSE) for the source code and CC0 for the images

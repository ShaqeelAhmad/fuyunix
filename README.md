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
\* I have only tried with tcc, gcc and clang; I mostly use tcc and might forget
to test with gcc / clang.

# Contributing
No.

# Portablility
I use Linux and can't guarantee that the program will work on other operating
systems

# Goals
- A small map with at least 10 levels
- Multiplayer support

# What will NOT be added
* Joystick / gamepad support. I don't have one to try it out.

# Files
`src/` directory contains the source code
`man/` contains the man page
`data/` contains the assets

# Installation
```sh
make
make install
```
use `make CC=cc` to specify a compiler or just edit the Makefile

replace cc with the compiler of your choice


# Usage
-v for version

don't pass any arguments to start the game.

# License
GPLv3 [LICENSE](LICENSE) for the source code and CC0 for the images

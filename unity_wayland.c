// cc -o fuyunix `pkg-config --cflags --libs wayland-client wayland-cursor xkbcommon cairo freetype2` -lm  unity_wayland.c
#include "xdg-decoration-unstable-client-protocol.h"
#include "xdg-shell-client-protocol.h"
#include "xdg-decoration-unstable-protocol.c"
#include "xdg-shell-protocol.c"

#include "src/game.c"
#include "src/platform_wayland.c"
#include "src/scfg.c"
#include "src/util.c"
#include "src/shm.c"

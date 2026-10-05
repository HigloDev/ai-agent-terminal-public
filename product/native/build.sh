#!/bin/sh
set -eu
cd "$(dirname "$0")"
mkdir -p lib/include/SDL2
cp /usr/include/x86_64-linux-gnu/SDL2/_real_SDL_config.h lib/include/SDL2/
aarch64-linux-gnu-gcc -std=gnu11 main.c markdown.c media.c vendor/md4c/md4c.c -o gm-native -O2 -Wall -Wextra -I/usr/include/SDL2 -Ilib/include -Llib -lSDL2 -lSDL2_ttf -Wl,--wrap=__libc_start_main -Wl,--unresolved-symbols=ignore-in-shared-libs
aarch64-linux-gnu-readelf --version-info gm-native | grep GLIBC

aarch64-linux-gnu-gcc discover.c -o gm-discover -O2 -Wall -Wextra -Wl,--wrap=__libc_start_main

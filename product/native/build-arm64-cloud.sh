#!/bin/sh
# Optional cross-build with supplied cloud sysroots. No device libraries are read.
set -eu
cd "$(dirname "$0")"
: "${GM_CROSS_ROOT:?Set cross compiler package root}"
: "${GM_ARM_SYSROOT:?Set target libc package root}"
: "${GM_ARM_SDL_HEADERS:?Set target SDL2 development package root}"
out=${GM_ARM_BUILD_DIR:-build/arm64}
mkdir -p "$out"
export LD_LIBRARY_PATH="$GM_CROSS_ROOT/usr/lib/x86_64-linux-gnu${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"
set -- -B"$GM_CROSS_ROOT/usr/lib/gcc-cross/aarch64-linux-gnu/14/" -B"$GM_CROSS_ROOT/usr/bin/" -B"$GM_ARM_SYSROOT/usr/aarch64-linux-gnu/lib/" --sysroot="$GM_ARM_SYSROOT" -I"$GM_ARM_SDL_HEADERS/usr/include/SDL2" -I"$GM_ARM_SDL_HEADERS/usr/include" -I"$GM_ARM_SDL_HEADERS/usr/include/aarch64-linux-gnu" -I"$GM_CROSS_ROOT/usr/include/SDL2" -I"$GM_ARM_SYSROOT/usr/aarch64-linux-gnu/include" -L"$GM_CROSS_ROOT/usr/lib/aarch64-linux-gnu" -L"$GM_ARM_SYSROOT/usr/aarch64-linux-gnu/lib"
"$GM_CROSS_ROOT/usr/bin/aarch64-linux-gnu-gcc-14" -std=gnu11 -O2 -Wall -Wextra main.c markdown.c media.c vendor/md4c/md4c.c "$@" -l:libSDL2-2.0.so.0 -l:libSDL2_ttf-2.0.so.0 -Wl,--wrap=__libc_start_main -Wl,--unresolved-symbols=ignore-in-shared-libs -o "$out/gm-native"
"$GM_CROSS_ROOT/usr/bin/aarch64-linux-gnu-gcc-14" -std=gnu11 -O2 -Wall -Wextra discover.c "$@" -Wl,--wrap=__libc_start_main -o "$out/gm-discover"
readelf --version-info "$out/gm-native"
printf 'ARM64 compiled; hardware/library compatibility still requires device acceptance.\n'

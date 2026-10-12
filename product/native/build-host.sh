#!/bin/sh
# Reproducible cloud/desktop SDL build; never uses a device or auth files.
set -eu
cd "$(dirname "$0")"
out=${GM_BUILD_DIR:-build/host}
mkdir -p "$out"
cc=${CC:-gcc}
if [ -n "${GM_NATIVE_SYSROOT:-}" ]; then
 set -- -I"$GM_NATIVE_SYSROOT/usr/include/SDL2" -I"$GM_NATIVE_SYSROOT/usr/include" -I"$GM_NATIVE_SYSROOT/usr/include/x86_64-linux-gnu" -L"$GM_NATIVE_SYSROOT/usr/lib/x86_64-linux-gnu"
else
 # shellcheck disable=SC2046
 set -- $(pkg-config --cflags --libs sdl2 SDL2_ttf)
fi
"$cc" -DGM_HOST_TEST -std=gnu11 -O2 -Wall -Wextra -Werror "$@" -c main.c -o "$out/main.o"
"$cc" -std=gnu11 -O2 -Wall -Wextra -Werror "$@" -c markdown.c -o "$out/markdown.o"
"$cc" -std=gnu11 -O2 -Wall -Wextra -Werror "$@" -c media.c -o "$out/media.o"
"$cc" -std=gnu11 -O2 -Wall -Wextra "$@" -c vendor/md4c/md4c.c -o "$out/md4c.o"
"$cc" "$out/main.o" "$out/markdown.o" "$out/media.o" "$out/md4c.o" "$@" -lSDL2 -lSDL2_ttf -Wl,-rpath,'$ORIGIN/lib' -o "$out/gm-native"
printf 'Host native build: %s/gm-native\n' "$out"

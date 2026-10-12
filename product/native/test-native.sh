#!/bin/sh
set -eu
cd "$(dirname "$0")"
out=${GM_TEST_DIR:-build/tests}
mkdir -p "$out"
cc=${CC:-gcc}
if [ -n "${GM_NATIVE_SYSROOT:-}" ]; then
 set -- -I"$GM_NATIVE_SYSROOT/usr/include/SDL2" -I"$GM_NATIVE_SYSROOT/usr/include" -I"$GM_NATIVE_SYSROOT/usr/include/x86_64-linux-gnu" -L"$GM_NATIVE_SYSROOT/usr/lib/x86_64-linux-gnu"
 export LD_LIBRARY_PATH="$GM_NATIVE_SYSROOT/usr/lib/x86_64-linux-gnu${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"
else
 # shellcheck disable=SC2046
 set -- $(pkg-config --cflags --libs sdl2 SDL2_ttf)
fi
export SDL_VIDEODRIVER=dummy
"$cc" -std=gnu11 -O2 -Wall -Wextra -Werror nav-input-test.c -o "$out/nav-input-test"
"$out/nav-input-test"
for name in markdown created-navigation questions-navigation appearance; do
 "$cc" -DGM_HOST_TEST -std=gnu11 -O1 -Wall -Wextra "$name-test.c" markdown.c media.c vendor/md4c/md4c.c "$@" -lSDL2 -lSDL2_ttf -o "$out/$name-test"
 "$out/$name-test" "$PWD/fonts/NotoSansSC-Regular.ttf"
done
"$cc" -DGM_HOST_TEST -std=gnu11 -O1 -Wall -Wextra media-state-test.c "$@" -lSDL2 -o "$out/media-state-test"
"$out/media-state-test"
GM_BUILD_DIR="$out/app" sh build-host.sh
"$out/app/gm-native" --self-test

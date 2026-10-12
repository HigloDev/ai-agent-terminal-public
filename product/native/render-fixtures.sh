#!/bin/sh
set -eu
cd "$(dirname "$0")"
out=${GM_CAPTURE_DIR:-build/screenshots}
mkdir -p "$out"
out=$(cd "$out" && pwd)
cc=${CC:-gcc}
if [ -n "${GM_NATIVE_SYSROOT:-}" ]; then
 set -- -I"$GM_NATIVE_SYSROOT/usr/include/SDL2" -I"$GM_NATIVE_SYSROOT/usr/include" -I"$GM_NATIVE_SYSROOT/usr/include/x86_64-linux-gnu" -L"$GM_NATIVE_SYSROOT/usr/lib/x86_64-linux-gnu"
 export LD_LIBRARY_PATH="$GM_NATIVE_SYSROOT/usr/lib/x86_64-linux-gnu${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"
else
 # shellcheck disable=SC2046
 set -- $(pkg-config --cflags --libs sdl2 SDL2_ttf)
fi
python3 - "$out" <<'PY'
from pathlib import Path
import hashlib,json,sys
files=sorted(set(Path('.').glob('*.c'))|set(Path('.').glob('*.h'))|set(Path('vendor/md4c').glob('*.[ch]'))|{Path('render-fixtures.sh')})
rows=[{'file':str(p),'sha256':hashlib.sha256(p.read_bytes()).hexdigest()} for p in files]
Path(sys.argv[1],'render-source-hashes.json').write_text(json.dumps(rows,indent=2)+'\n')
PY
"$cc" -DGM_HOST_TEST -std=gnu11 -O1 -Wall -Wextra ui-fixtures.c markdown.c media.c vendor/md4c/md4c.c "$@" -lSDL2 -lSDL2_ttf -o "$out/ui-fixtures"
SDL_VIDEODRIVER=dummy "$out/ui-fixtures" "$PWD/fonts/NotoSansSC-Regular.ttf" "$out" "$PWD/skins"
python3 - "$out" <<'PY'
from pathlib import Path
from PIL import Image
import sys,json,hashlib
out=Path(sys.argv[1])
for row in json.loads((out/'render-source-hashes.json').read_text()):
    assert hashlib.sha256(Path(row['file']).read_bytes()).hexdigest()==row['sha256'], 'source changed during rendering: '+row['file']
(out/'renderer-binary.sha256').write_text(hashlib.sha256((out/'ui-fixtures').read_bytes()).hexdigest()+'  ui-fixtures\n')
for f in Path(sys.argv[1]).glob('*.bmp'):
    im=Image.open(f).convert('RGB')
    assert im.size==(1280,720)
    im.save(f.with_suffix('.png'))
    f.unlink()
PY

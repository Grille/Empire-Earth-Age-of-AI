#!/bin/sh
# Builds eeaimod.dll (32-bit) with llvm-mingw (tools/llvm-mingw, or set CC to another i686 mingw compiler).
# usage: sh build.sh [install]   - "install" also copies the DLL into the game folder (two levels up).
# -mstackrealign: the game calls our hooks with a 4-byte aligned stack; vectorized code (movdqa to the stack)
# would crash without it.
cd "$(dirname "$0")"
CC=${CC:-../tools/llvm-mingw/bin/i686-w64-mingw32-clang}
# the layout hashes of the saved-game blocks (persist.c): persist_layout.h, kept in git for a build without Python
PY=../venv/Scripts/python.exe
[ -x "$PY" ] || PY=$(command -v python3 || command -v python)
if [ -n "$PY" ]; then "$PY" ../scripts/persist_layout.py || exit 1
else echo "no Python: persist_layout.h left as it is (run scripts/persist_layout.py after changing a saved variable)"; fi
"$CC" -O2 -mstackrealign -Wall -Wextra -shared -static -o eeaimod.dll eeaimod.c -lgdi32 -ldxguid -lwinmm -lwinhttp -Wl,--image-base,0x6E000000 || exit 1
# dinput.dll (optional): the game's settings in eesettings.ini instead of the registry (eesettings.c)
"$CC" -O2 -Wall -Wextra -shared -static -o dinput.dll eesettings.c eesettings.def -ladvapi32 -luser32 || exit 1
if [ "$1" = install ]; then
    cp eeaimod.dll ../../eeaimod.dll && echo "installed to $(cd ../.. && pwd)/eeaimod.dll"
    # another program's dinput.dll is never replaced (then the settings simply stay in the registry)
    if [ ! -f ../../dinput.dll ] || grep -q EEAimodSettings ../../dinput.dll; then
        cp dinput.dll ../../dinput.dll && echo "installed to $(cd ../.. && pwd)/dinput.dll (settings in eesettings.ini)"
    else echo "kept another program's dinput.dll - the game's settings stay in the registry"; fi
fi

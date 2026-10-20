#!/bin/sh
# Builds eeaimod.dll (32-bit) with llvm-mingw (tools/llvm-mingw, or set CC to another i686 mingw compiler).
# usage: sh build.sh [install]   - "install" also copies the DLL into the game folder (scripts/game_dir.sh).
# -mstackrealign: the game calls our hooks with a 4-byte aligned stack; vectorized code (movdqa to the stack)
# would crash without it.
cd "$(dirname "$0")"
CC=${CC:-../tools/llvm-mingw/bin/i686-w64-mingw32-clang}
# the layout hashes of the saved-game blocks (persist.c): persist_layout.h, kept in git for a build without Python
PY=../venv/Scripts/python.exe
[ -x "$PY" ] || PY=$(command -v python3 || command -v python)
if [ -n "$PY" ]; then "$PY" ../scripts/persist_layout.py && "$PY" ../scripts/gen_cmd_verbs.py || exit 1
else echo "no Python: persist_layout.h and cmd_verbs.h left as they are (scripts/persist_layout.py after changing a saved variable, gen_cmd_verbs.py after a new pipe command)"; fi
"$CC" -O2 -mstackrealign -Wall -Wextra -shared -static -o eeaimod.dll eeaimod.c -lgdi32 -ldxguid -lwinmm -lwinhttp -Wl,--image-base,0x6E000000 || exit 1
# dinput.dll (optional): the game's settings in eesettings.ini instead of the registry (eesettings.c)
"$CC" -O2 -Wall -Wextra -shared -static -o dinput.dll eesettings.c eesettings.def -ladvapi32 -luser32 || exit 1
if [ "$1" = install ]; then   # the working tree's build (scripts/install.sh: the committed one) into the game folder
    REPO="$(cd .. && pwd)"; . ../scripts/game_dir.sh   # GAME
    cp eeaimod.dll "$GAME/eeaimod.dll" && echo "installed to $GAME/eeaimod.dll"
    # another program's dinput.dll is never replaced (then the settings simply stay in the registry)
    if [ ! -f "$GAME/dinput.dll" ] || grep -q EEAimodSettings "$GAME/dinput.dll"; then
        cp dinput.dll "$GAME/dinput.dll" && echo "installed to $GAME/dinput.dll (settings in eesettings.ini)"
    else echo "kept another program's dinput.dll - the game's settings stay in the registry"; fi
fi

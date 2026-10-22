#!/bin/sh
# Builds eeaimod.dll (32-bit) with llvm-mingw (tools/llvm-mingw, or set CC to another i686 mingw compiler).
# usage: sh build.sh [install]   - "install" also copies the DLL into the game folder (scripts/game_dir.sh).
# -mstackrealign: the game calls our hooks with a 4-byte aligned stack; vectorized code (movdqa to the stack)
# would crash without it.
# -fno-strict-aliasing: the mod reads the game's memory through whatever type a field has (FIELD: an int, then a
# float or a pointer of another kind at the same place) - the optimizer must not assume two pointer types never meet
# (clang tells even pointer types apart: ee_player * from uint8_t *).
# -Wformat=2 (the printf-like functions marked PRINTF_LIKE: their calls checked), -Wstrict-prototypes (f() takes any
# arguments before C23 - f(void) none), -Wimplicit-fallthrough (a case running into the next), -Wvla (an array
# sized at run time on the stack): C's legacy traps the code has none of, kept out
# --exclude-all-symbols: the DLL exports nothing (without an export list mingw exports every name without static -
# dxguid's 1370 GUIDs and a few hooks; nothing looks them up: the hooks are reached by patched addresses)
cd "$(dirname "$0")"
CC=${CC:-../tools/llvm-mingw/bin/i686-w64-mingw32-clang}
# the layout hashes of the saved-game blocks (persist.c): persist_layout.h, kept in git for a build without Python
PY=../venv/Scripts/python.exe
[ -x "$PY" ] || PY=$(command -v python3 || command -v python)
# the folders' rules (which part may use which: scripts/layers.py) - a use across them fails the build
# and the C style the compiler leaves open (scripts/style.py: int* a, b - only a is a pointer)
if [ -n "$PY" ]; then "$PY" ../scripts/persist_layout.py && "$PY" ../scripts/gen_cmd_verbs.py && "$PY" ../scripts/layers.py --quiet && "$PY" ../scripts/addr_sort.py --check && "$PY" ../scripts/style.py || exit 1
else echo "no Python: persist_layout.h and cmd_verbs.h left as they are, the folders' rules unchecked (scripts/persist_layout.py after changing a saved variable, gen_cmd_verbs.py after a new pipe command)"; fi
"$CC" -O2 -mstackrealign -fno-strict-aliasing -Wall -Wextra -Wformat=2 -Wstrict-prototypes -Wimplicit-fallthrough -Wvla -shared -static -I. -o eeaimod.dll eeaimod.c -lgdi32 -ldxguid -lwinmm -lwinhttp -Wl,--image-base,0x6E000000 -Wl,--exclude-all-symbols || exit 1
# dinput.dll (optional): the game's settings in eesettings.ini instead of the registry (dinput/eesettings.c)
"$CC" -O2 -Wall -Wextra -Wformat=2 -Wstrict-prototypes -Wimplicit-fallthrough -Wvla -shared -static -o dinput.dll dinput/eesettings.c dinput/eesettings.def -ladvapi32 -luser32 || exit 1
if [ "$1" = install ]; then   # the working tree's build (scripts/install.sh: the committed one) into the game folder
    REPO="$(cd .. && pwd)"; . ../scripts/game_dir.sh   # GAME
    cp eeaimod.dll "$GAME/eeaimod.dll" && echo "installed to $GAME/eeaimod.dll"
    # another program's dinput.dll is never replaced (then the settings simply stay in the registry)
    if [ ! -f "$GAME/dinput.dll" ] || grep -q EEAimodSettings "$GAME/dinput.dll"; then
        cp dinput.dll "$GAME/dinput.dll" && echo "installed to $GAME/dinput.dll (settings in eesettings.ini)"
    else echo "kept another program's dinput.dll - the game's settings stay in the registry"; fi
fi

#!/bin/sh
# Installs the committed build into the game folder: eeaimod.dll (and the mod's dinput.dll, aimod_config.exe) built from a clean
# `git archive HEAD` - never from the working tree, which may hold another session's unfinished changes. A DLL a
# running game has loaded can't be replaced: it builds first (HEAD as it is now), then reserves the game folder
# (reserve.sh: no test starts a game any more, the running ones - and the player's - are waited for) and copies.
# usage: sh scripts/build/install.sh            (prints "installed <commit>" or why not; the game folder: game_dir.sh)
. "$(dirname "$0")/../test/reserve.sh"   # GAME, REPO, reserve_begin/end
A="$REPO"
G="$GAME"
B="${A:?no repository}/local/work/install_build"
rm -rf "$B" && mkdir -p "$B"
REV=$(cd "$A" && git rev-parse --short HEAD)
(cd "$A" && git archive "$REV" | tar -x -C "$B") || exit 1
(cd "$B/dll" && PY="$A/local/venv/Scripts/python.exe" CC="$A/local/tools/llvm-mingw/bin/i686-w64-mingw32-clang" sh build.sh > build.txt 2>&1) || { cat "$B/dll/build.txt"; exit 1; }
reserve_begin "install $REV"
cp "$B/dll/eeaimod.dll" "$G/eeaimod.dll" || exit 1
# another program's dinput.dll is never replaced (then the game's settings stay in the registry)
if [ ! -f "$G/dinput.dll" ] || grep -q EEAimodSettings "$G/dinput.dll"; then cp "$B/dll/dinput.dll" "$G/dinput.dll" || exit 1; fi
# the settings program: one the player has open can't be replaced - said, and the exit code 3 (install_latest.sh waits
# for it to close and installs again)
cfg=0
cp "$B/dll/aimod_config.exe" "$G/aimod_config.exe" 2>/dev/null || { echo "aimod_config.exe is open - NOT replaced" >&2; cfg=3; }
reserve_end
echo "installed $REV"
exit $cfg

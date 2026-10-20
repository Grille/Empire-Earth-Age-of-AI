#!/bin/sh
# Installs the committed build into the game folder: eeaimod.dll (and the mod's dinput.dll) built from a clean
# `git archive HEAD` - never from the working tree, which may hold another session's unfinished changes. A DLL a
# running game has loaded can't be replaced: it builds first (HEAD as it is now), then waits until no Empire Earth.exe
# runs (the player's game or a test's) and copies at once - building while waiting lost every time beside test runs.
# usage: sh scripts/install.sh            (prints "installed <commit>" or why not; the game folder: game_dir.sh)
. "$(dirname "$0")/game_dir.sh"   # GAME, REPO
A="$REPO"
G="$GAME"
B="${A:?no repository}/work/install_build"
idle() { ! tasklist //FI "IMAGENAME eq Empire Earth.exe" 2>/dev/null | grep -q "Empire Earth"; }
rm -rf "$B" && mkdir -p "$B"
REV=$(cd "$A" && git rev-parse --short HEAD)
(cd "$A" && git archive "$REV" | tar -x -C "$B") || exit 1
[ -d "$A/venv" ] && cp -r "$A/venv" "$B/venv"   # build.sh's generators (persist_layout.py, gen_cmd_verbs.py)
(cd "$B/dll" && CC="$A/tools/llvm-mingw/bin/i686-w64-mingw32-clang" sh build.sh > build.txt 2>&1) || { cat "$B/dll/build.txt"; exit 1; }
until idle; do sleep 5; done
cp "$B/dll/eeaimod.dll" "$G/eeaimod.dll" || { echo "not installed: a game started just now - run it again"; exit 1; }
# another program's dinput.dll is never replaced (then the game's settings stay in the registry)
if [ ! -f "$G/dinput.dll" ] || grep -q EEAimodSettings "$G/dinput.dll"; then cp "$B/dll/dinput.dll" "$G/dinput.dll" || exit 1; fi
echo "installed $REV"

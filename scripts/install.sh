#!/bin/sh
# Installs the committed build into the game folder: eeaimod.dll (and the mod's dinput.dll) built from a clean
# `git archive HEAD` - never from the working tree, which may hold another session's unfinished changes. A DLL a
# running game has loaded can't be replaced: it waits until no Empire Earth.exe runs (the player's game or a test's),
# builds, and copies only if still none runs. HEAD is taken when it builds, so a commit made while it waits goes in.
# usage: sh scripts/install.sh            (from _aimod; prints "installed <commit>" or why not)
A="$(cd "$(dirname "$0")/.." && pwd)"
G="$(cd "$A/.." && pwd)"
B="$A/work/install_build"
idle() { ! tasklist //FI "IMAGENAME eq Empire Earth.exe" 2>/dev/null | grep -q "Empire Earth"; }
until idle; do sleep 30; done
rm -rf "$B" && mkdir -p "$B"
REV=$(cd "$A" && git rev-parse --short HEAD)
(cd "$A" && git archive "$REV" | tar -x -C "$B") || exit 1
[ -d "$A/venv" ] && cp -r "$A/venv" "$B/venv"   # build.sh's generators (persist_layout.py, gen_cmd_verbs.py)
(cd "$B/dll" && CC="$A/tools/llvm-mingw/bin/i686-w64-mingw32-clang" sh build.sh > build.txt 2>&1) || { cat "$B/dll/build.txt"; exit 1; }
idle || { echo "not installed: a game started while it built - run it again"; exit 1; }
cp "$B/dll/eeaimod.dll" "$G/eeaimod.dll" || exit 1
# another program's dinput.dll is never replaced (then the game's settings stay in the registry)
if [ ! -f "$G/dinput.dll" ] || grep -q EEAimodSettings "$G/dinput.dll"; then cp "$B/dll/dinput.dll" "$G/dinput.dll" || exit 1; fi
echo "installed $REV"

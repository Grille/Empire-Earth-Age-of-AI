#!/bin/sh
# Installs the committed build into the game folder: ageofai.dll (and ageofai_config.exe, ageofai_check.exe) built from a clean
# `git archive HEAD` - never from the working tree, which may hold another session's unfinished changes.
# A FILE IN USE IS MOVED ASIDE, NOT WAITED FOR: a DLL a running game has loaded (or a running program) can't be written
# over, but Windows lets it be renamed and deleted - the game runs on from the old image, its next start loads the new
# file. So the install neither waits for the player's game nor for the tests' (it did until 2026-11-04: the player
# started his game while the install waited for a test's, the copy then failed "busy" and nothing tried again - he
# kept finding things fixed minutes before). The reservation (reserve.sh) is still taken, for the moment of the swap:
# no test starts a game meanwhile, but the running ones aren't waited for (RESERVE_NO_WAIT).
# usage: sh scripts/build/install.sh            (prints "installed <commit>" or why not; the game folder: game_dir.sh)
. "$(dirname "$0")/../test/reserve.sh"   # GAME, REPO, reserve_begin/end
A="$REPO"
G="$GAME"
B="${A:?no repository}/local/work/install_build"
rm -rf "$B" && mkdir -p "$B"
REV=$(cd "$A" && git rev-parse --short HEAD)
(cd "$A" && git archive "$REV" | tar -x -C "$B") || exit 1
(cd "$B" && PY="$A/local/venv/Scripts/python.exe" CC="$A/local/tools/llvm-mingw/bin/i686-w64-mingw32-clang" sh build.sh > dll/build.txt 2>&1) || { cat "$B/dll/build.txt"; exit 2; }
RESERVE_NO_WAIT=1 reserve_begin "install $REV"
. "$(dirname "$0")/mod_dir.sh"
mod_dir_move "$G" || exit 1   # ageofai.ini, eesettings.ini, the log and the dumps into <game>\ageofai\, once
# <built> <name in the game folder>: the new file staged in ageofai\ beside it (not in the game folder: the engine
# loads every DLL there), the old one renamed into ageofai\ (works while it is loaded or running), the new one renamed
# in - the name is missing only between two renames - and the old one deleted (also while loaded). A failure puts
# the old one back.
replace() {
    t="$G/ageofai/$2.new"; o="$G/ageofai/$2.old"
    rm -f "$t" "$o" 2>/dev/null
    [ -e "$o" ] && o="$o.$$"   # a .old of before that can't go
    cp "$1" "$t" || return 1
    if [ -e "$G/$2" ]; then mv -f "$G/$2" "$o" || { rm -f "$t"; return 1; }; fi
    if ! mv "$t" "$G/$2"; then [ -e "$o" ] && mv "$o" "$G/$2"; return 1; fi
    rm -f "$o" 2>/dev/null
    return 0
}
replace "$B/dll/ageofai.dll" ageofai.dll || { echo "ageofai.dll NOT replaced" >&2; exit 1; }
mod_old_remove "$G"   # eeaimod.dll and aimod_config.exe, the old names: two copies of the mod would both load
replace "$B/dll/ageofai_config.exe" ageofai_config.exe || { echo "ageofai_config.exe NOT replaced" >&2; exit 1; }
# the .eeai checker (the player 2026-11-02: in the game folder - for modders, not in the release zip: a download of its own)
replace "$B/dll/ageofai_check.exe" ageofai_check.exe || { echo "ageofai_check.exe NOT replaced" >&2; exit 1; }
[ -e "$G/ageofai/texfix.eeai" ] || cp "$B/config/texfix.eeai" "$G/ageofai/texfix.eeai"   # once: the player's own list stays
reserve_end
(cd "$A" && git rev-parse HEAD) > "$A/local/work/installed.txt"   # install_wait.sh reads it
echo "installed $REV"

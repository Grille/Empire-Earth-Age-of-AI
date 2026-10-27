#!/bin/sh
# The mod's files in the game folder's ageofai\ (the game folder itself keeps only ageofai.dll, dinput.dll and
# ageofai_config.exe). A game folder set up before has them under their old names - beside the game (before
# 2026-10-27) or in aimod\ (until the name Age of AI, 2026-10-27) - moved in once, by every install (install.sh,
# dll/build.sh install) before the new DLL can look for them: aimod.ini -> ageofai.ini, aimod_econ.csv ->
# ageofai_log.csv, aimod_dumps -> ageofai_dumps, aimod_autotest.ini -> ageofai_autotest.ini, eesettings.ini and
# personalities\ as they are; the state files beside the saved games, *.aimod -> *.ageofai. A file already there under
# its new name is left alone, and so is its old one. The old personality folder aimod_ai isn't moved: a developer's
# game reads the repository's (ai_dir), a copy of them would be out of date.
# usage: . scripts/build/mod_dir.sh; mod_dir_move <game folder>; after the new DLL is in: mod_old_remove <game folder>
mod_move() {   # <from> <to>: once, never over a file already there
    if [ -e "$1" ] && [ ! -e "$2" ]; then mv "$1" "$2" && echo "moved $1 to $2"; fi
}
mod_dir_move() {
    [ -n "$1" ] || return 1
    mkdir -p "$1/ageofai" || return 1
    for d in "$1/aimod" "$1"; do   # aimod\ first: where both have a file, its is the newer
        mod_move "$d/aimod.ini" "$1/ageofai/ageofai.ini"
        mod_move "$d/eesettings.ini" "$1/ageofai/eesettings.ini"
        mod_move "$d/aimod_econ.csv" "$1/ageofai/ageofai_log.csv"
        mod_move "$d/aimod_econ.old.csv" "$1/ageofai/ageofai_log.old.csv"
        mod_move "$d/aimod_dumps" "$1/ageofai/ageofai_dumps"
        mod_move "$d/aimod_autotest.ini" "$1/ageofai/ageofai_autotest.ini"
    done
    mod_move "$1/aimod/personalities" "$1/ageofai/personalities"
    rmdir "$1/aimod" 2>/dev/null   # only when nothing else was left in it
    if [ -d "$1/Data/Saved Games" ]; then
        find "$1/Data/Saved Games" -name '*.aimod' | while IFS= read -r f; do mod_move "$f" "${f%.aimod}.ageofai"; done
    fi
    return 0
}
# The old names' programs: the engine loads every DLL in its folder, so an eeaimod.dll left beside ageofai.dll would
# install the mod a second time over the same game.
mod_old_remove() {
    [ -n "$1" ] || return 1
    for f in eeaimod.dll aimod_config.exe; do
        if [ -e "$1/$f" ]; then rm -f "$1/$f" && echo "removed the old $f" || echo "the old $1/$f is in use - delete it by hand" >&2; fi
    done
    return 0
}

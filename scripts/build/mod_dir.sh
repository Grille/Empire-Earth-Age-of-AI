#!/bin/sh
# The mod's files in the game folder's aimod\ (2026-10-27: the game folder itself keeps only eeaimod.dll, dinput.dll
# and aimod_config.exe): a game folder set up before has them beside the game - moved in once, by every install
# (install.sh, dll/build.sh install) before the new DLL can look for them. A file already in aimod\ is left alone, and
# so is it beside the game. The old personality folder aimod_ai isn't moved into aimod\personalities: a developer's
# game reads the repository's (ai_dir), a copy of them would be out of date.
# usage: . scripts/build/mod_dir.sh; mod_dir_move <game folder>
mod_dir_move() {
    [ -n "$1" ] || return 1
    mkdir -p "$1/aimod" || return 1
    for f in aimod.ini eesettings.ini aimod_econ.csv aimod_econ.old.csv aimod_dumps; do
        if [ -e "$1/$f" ] && [ ! -e "$1/aimod/$f" ]; then mv "$1/$f" "$1/aimod/$f" && echo "moved $f into $1/aimod/"; fi
    done
    return 0
}

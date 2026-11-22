#!/bin/sh
# The game under Wine, on this PC: WSL2's Linux (distro WSL_DISTRO, default Ubuntu-24.04) - for the players on Wine
# (Linux, Android's Wine apps). Nothing of it touches the game folder: the game is a copy in local/work/wine_game,
# the Wine prefix's C:\ee a copy of that (/root/eeprefix in the distro). The Linux side: scripts/test/wine_in.sh.
# usage: sh scripts/test/wine.sh setup                  Wine and the rest into the distro (once; `wsl --install` first)
#        sh scripts/test/wine.sh game                   local/work/wine_game: the game folder without the mod's files,
#                                                       logs and saves (aitest.ees kept), HEAD's build installed there
#        sh scripts/test/wine.sh prefix                 the prefix: that copy as C:\ee, the game's registry key with C:\ee
#        sh scripts/test/wine.sh run <seconds> "<ini>"  a test game (the test ini's keys as -IniExtra; headless=0 for
#                                                       the real window - it is in Xvfb, never on the screen)
# DLL=<path>: the run plays that ageofai.dll (copied into local/work/wine_game). The log: local/work/wine_log.csv,
# screenshots: local/work/wine_shots/ (SHOT=n seconds apart). docs/testing.md "Wine".
# Checked 2026-11-22 (Wine 11.0, Ubuntu 24.04): Wine's Direct3D on llvmpipe draws the own renderer's game picture.
cd "$(dirname "$0")/../.." || exit 1
D=${WSL_DISTRO:-Ubuntu-24.04}
W=local/work/wine_game
in_wsl() { MSYS_NO_PATHCONV=1 WSLENV="SHOT:WINEDEBUG" wsl.exe -d "$D" -u root --cd "$(wslpath_of "$PWD")" -- bash scripts/test/wine_in.sh "$@"; }
wslpath_of() { printf '/mnt/%s%s' "$(echo "$1" | cut -c2 | tr 'A-Z' 'a-z')" "$(echo "$1" | cut -c3-)"; }
case "${1:-}" in
setup)
    in_wsl setup ;;
game)
    REPO="$PWD"; . scripts/game_dir.sh
    [ -n "${GAME:-}" ] && [ -f "$GAME/$GAME_EXE" ] || { echo "no game folder (scripts/game_dir.sh): '${GAME:-}'"; exit 1; }
    rm -rf "$W"; mkdir -p "$W/Data/Saved Games"
    for f in "$GAME"/*; do
        case "$(basename "$f")" in
        _aimod|ageofai|ageofai.dll|ageofai_check.exe|ageofai_config.exe|Screenshots|work|_wonHTTPCache|Data|*.scn) ;;
        *) cp -r "$f" "$W/" ;;
        esac
    done
    for d in "$GAME"/Data/*; do [ "$(basename "$d")" = "Saved Games" ] || cp -r "$d" "$W/Data/"; done
    cp -p "$GAME/Data/Saved Games/aitest.ees" "$W/Data/Saved Games/"
    EE_GAME_DIR="$(cygpath -w "$PWD/$W")" sh scripts/build/install.sh || exit 1
    [ -f "$W/ageofai/ageofai.ini" ] || cp config/ageofai.ini "$W/ageofai/"
    [ -d "$W/ageofai/personalities" ] || cp -r config/personalities "$W/ageofai/"
    du -sh "$W" ;;
prefix)
    # the game's settings key (HKCU\Software\SSSI\Empire Earth) with the install's place at C:\ee
    R="$PWD/local/work/wine_game.reg"
    reg.exe export 'HKCU\SOFTWARE\SSSI' "$(cygpath -w "$R")" //y > /dev/null || exit 1
    powershell.exe -NoProfile -Command "\$t = Get-Content '$(cygpath -w "$R")'; \$t -replace '\"Installed From Volume\"=\".*\"', '\"Installed From Volume\"=\"C:\"' -replace '\"Installed From Directory\"=\".*\"', '\"Installed From Directory\"=\"\\\\EE\\\\\"' | Set-Content -Encoding Unicode '$(cygpath -w "$R")'"
    in_wsl prefix "$(wslpath_of "$R")" ;;
run)
    [ -n "${DLL:-}" ] && { cp "$DLL" "$W/ageofai.dll" || exit 1; }
    in_wsl run "${2:-120}" "${3:-}" ;;
*)
    sed -n '4,13p' "$0"; exit 1 ;;
esac

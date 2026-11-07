#!/bin/sh
# Start the game headless (off-screen window, virtual mouse, muted) and open the scenario editor.
# usage: [SLOT=n] sh scripts/test/editor_headless.sh [editor_resolution]      e.g. game | aoc | 1920x1080
#   SLOT=n: in test slot n, beside the player's game (eectl.py and run_test.ps1 take SLOT from the environment)
#   EXTRA=k=v,...: more test ini keys (wall_slopes=1,wall_joins=1)
# Then drive it with: eectl.py vmouse/vclick/wclick/shot (see docs/testing.md). Quit with: eectl.py quit
cd "$(dirname "$0")/../.." || exit 1
REPO="$(pwd)"; . scripts/game_dir.sh   # GAME: its log is in the game folder's ageofai\ (slot 0)
res=${1:-game}
py=local/venv/Scripts/python
if [ -z "$SLOT" ] && tasklist | grep -qi "empire earth"; then echo "a game is running - close it first (or SLOT=n)"; exit 1; fi
(powershell -ExecutionPolicy Bypass -File scripts/test/run_test.ps1 -Save nosuch.ees -TimeoutSec 3600 -Tag _editor \
    -IniExtra "headless=1,editor_resolution=$res${EXTRA:+,$EXTRA}" > local/work/editor_headless.txt 2>&1 &)
# main menu: wait until the control pipe answers and a frame can be taken
for i in $(seq 1 60); do sleep 2; $py scripts/test/eectl.py shot menu >/dev/null 2>&1 && break; done
sleep 3
log="$GAME/ageofai/ageofai_log.csv"
[ -n "$SLOT" ] && log="local/work/slot$SLOT/ageofai_log.csv"
# The way in, by the form the game shows (the pipe's form: its dbuiforms.dat row): a click is never blind. A click
# often doesn't take (the button shows its tooltip only), so each round looks again; and the main menu's Game Tools
# button lies right under the Game Tools menu's Campaign Editor, which its release can press too - from there its own
# Scenario Editor button. (Clicking "back" at 512,680 blindly, as before, was Exit Game in the main menu.)
ctl() { $py scripts/test/eectl.py "$@"; }
form() { ctl form 2>/dev/null | sed -n 's/^form \(-*[0-9]*\).*/\1/p'; }   # the editor's own form: -1
press() {   # x y: onto the button in small steps (a single move can be missed), then the click
    for d in 8 4 0; do ctl vmouse $(($1 - d)) $(($2 - d)) >/dev/null; sleep 0.3; done
    ctl vclick "$1" "$2" >/dev/null
}
opened() {   # the mod's editor log line; the original editor (ee) writes none: a form that is no menu after the press
    if [ "$res" = "ee" ]; then [ -n "$asked" ] && f=$(form) && [ -n "$f" ] && [ "$f" != 12 ] && [ "$f" != 9 ] && [ "$f" != 24 ]
    else grep -aq "# editor: building\|# editor: unit search box" "$log"; fi
}
asked=
for try in $(seq 1 20); do
    opened && break
    case "$(form)" in
        12) press 512 472 ;;                 # main menu: Game Tools
        9) press 512 370; asked=1 ;;         # Game Tools: Scenario Editor
        24) press 256 727; asked=1 ;;        # Campaign Editor: Scenario Editor (bottom left)
    esac
    sleep 3
done
if ! opened; then echo "could not open the editor (form $(form))"; exit 1; fi
sleep 8
[ "$res" = "ee" ] && { echo "editor open (original layout) - check with: eectl.py shot <name>"; exit 0; }
grep -a "# editor: \(building\|resolution [0-9]\)\|# exception" "$log"; echo "editor open"

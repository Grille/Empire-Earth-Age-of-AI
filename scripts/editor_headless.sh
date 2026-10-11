#!/bin/sh
# Start the game headless (off-screen window, virtual mouse, muted) and open the scenario editor.
# usage: sh scripts/editor_headless.sh [editor_resolution]      e.g. game | aoc | 1920x1080
# Then drive it with: eectl.py vmouse/vclick/wclick/shot (see docs/testing.md). Quit with: eectl.py quit
cd "$(dirname "$0")/.." || exit 1
res=${1:-game}
py=venv/Scripts/python
if tasklist | grep -qi "empire earth"; then echo "a game is running - close it first"; exit 1; fi
(powershell -ExecutionPolicy Bypass -File scripts/run_test.ps1 -Save nosuch.ees -TimeoutSec 3600 -Tag _editor \
    -IniExtra "headless=1,editor_resolution=$res" > work/editor_headless.txt 2>&1 &)
# main menu: wait until the control pipe answers and a frame can be taken
for i in $(seq 1 60); do sleep 2; $py scripts/eectl.py shot menu >/dev/null 2>&1 && break; done
sleep 3
log=../aimod_econ.csv
for try in 1 2 3 4 5; do
    $py scripts/eectl.py vmouse 500 460 >/dev/null; sleep 0.5
    $py scripts/eectl.py vclick 512 472 >/dev/null; sleep 4     # main menu: game tools
    $py scripts/eectl.py vmouse 500 360 >/dev/null; sleep 0.5
    $py scripts/eectl.py vclick 512 370 >/dev/null              # game tools: scenario editor
    for i in $(seq 1 15); do sleep 1; grep -aq "# editor: building" $log && break; done
    if grep -aq "# editor: building" $log; then
        sleep 8; grep -a "# editor: \(building\|resolution [0-9]\)\|# exception" $log; echo "editor open"; exit 0
    fi
    $py scripts/eectl.py vclick 512 680 >/dev/null; sleep 3     # back to the main menu (bottom button) and retry
done
echo "could not open the editor"; exit 1

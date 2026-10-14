#!/bin/sh
# Head-to-head test: in every game some AIs play variant B (v_* keys), the others the settings (A); the B seats swap
# between runs (odd seeds P2+P4, even seeds P3+P5 with 4 AIs), so map spot and opponent even out.
# usage: sh scripts/h2h.sh <minutes> <scenario> "<base ini keys>" "<B keys>"   (B keys: v_late_targets=15,...)
#   REPEAT=n (default 6) runs with game_seed 1..n; NAME=label (default h2h). Score: scripts/h2h.py work/*_h2h<stamp>_*
cd "$(dirname "$0")/.."
MIN=$1; SCEN=$2; BASE=$3; B=$4
REPEAT=${REPEAT:-6}; NAME=${NAME:-h2h}
[ -f "scenarios/$SCEN.txt" ] || { echo "no scenarios/$SCEN.txt"; exit 1; }
STAMP=$(date +%Y%m%d_%H%M)
for r in $(seq 1 $REPEAT); do
    seats=24; [ $((r % 2)) -eq 0 ] && seats=35
    ini="$BASE,teams=ffa,ai_cheat=0,ai_ignore=1,headless=1,turbo=1,quit_minutes=$MIN,game_seed=$r,setup=_aimod/scenarios/$SCEN.txt"
    ini="$ini,variant_players=$seats,$B"
    powershell -NoProfile -ExecutionPolicy Bypass -File scripts/run_test.ps1 -Save newgame.ees \
        -TimeoutSec $((MIN * 12 + 120)) -Tag "_h2h${STAMP}_${NAME}_${SCEN}-r$r" -IniExtra "$ini" | tail -1
done
venv/Scripts/python.exe scripts/h2h.py work/autotest_*_h2h${STAMP}_*.csv
echo H2H DONE $STAMP

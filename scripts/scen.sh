#!/bin/sh
# A/B test on a built scenario: a base map plus a setup file (control-pipe commands run at 2 s, e.g. spawn).
# usage: sh scripts/scen.sh <minutes> <scenario> "<base ini keys>" <variant>...   (variant = name:ini,keys)
#   scenario = scenarios/<scenario>.txt; REPEAT=n (default 5) runs with game_seed 1..n per variant.
# e.g. sh scripts/scen.sh 30 towers_on_path "random_map=Continental,map_size=0,ai_count=2,seed=101,start_epoch=3" \
#          "old:attack_leash=0,attack_weak_spot_radius=25,avoid_towers_margin=3" "new:"
cd "$(dirname "$0")/.."
MIN=$1; SCEN=$2; BASE=$3; shift 3
REPEAT=${REPEAT:-5}
[ -f "scenarios/$SCEN.txt" ] || { echo "no scenarios/$SCEN.txt"; exit 1; }
STAMP=$(date +%Y%m%d_%H%M)
for v in "$@"; do
    name=${v%%:*}; extra=${v#*:}
    for r in $(seq 1 $REPEAT); do
        ini="$BASE,teams=ffa,ai_cheat=0,ai_ignore=1,headless=1,turbo=1,quit_minutes=$MIN,game_seed=$r,setup=_aimod/scenarios/$SCEN.txt"
        [ -n "$extra" ] && ini="$ini,$extra"
        powershell -NoProfile -ExecutionPolicy Bypass -File scripts/run_test.ps1 -Save newgame.ees \
            -TimeoutSec $((MIN * 12 + 120)) -Tag "_suite${STAMP}_${name}_${SCEN}-r$r" -IniExtra "$ini" | tail -1
    done
done
venv/Scripts/python.exe scripts/score.py -q work/autotest_*_suite${STAMP}_*.csv
echo SCEN DONE $STAMP

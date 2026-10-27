#!/bin/sh
# Head-to-head test: in every game some AIs play variant B (v_* keys), the others the settings (A); the B seats swap
# between runs (odd seeds P2+P4, even seeds P3+P5 with 4 AIs), so map spot and opponent even out.
# usage: sh scripts/test/h2h.sh <minutes> <scenario> "<base ini keys>" "<B keys>"   (B keys: v_late_targets=15,...)
#   REPEAT=n (default 6) runs with game_seed 1..n; NAME=label (default h2h). Score: scripts/results/h2h.py local/work/*_h2h<stamp>_*
#   JOBS=n: n games at a time (scripts/test/jobs.sh).
#   MIRROR=1: every seed twice, the B seats swapped (P2+P4, then P3+P5) - map spot and seed even out in each pair. Without
#   it the seats alternate by the seed's parity: a control with B = A (2026-10-27, v_threat_memory=0, Continental late, 24
#   seeds) came out B - A alive -0.35 (se 0.10), workers -21 (7.7) - the seeds' spots don't even out.
cd "$(dirname "$0")/../.."
. scripts/test/jobs.sh
MIN=$1; SCEN=$2; BASE=$3; B=$4
REPEAT=${REPEAT:-6}; NAME=${NAME:-h2h}; SEED0=${SEED0:-0}   # SEED0: game_seed = SEED0 + 1..n
[ -f "tests/scenarios/$SCEN.eeai" ] || { echo "no tests/scenarios/$SCEN.eeai"; exit 1; }
STAMP=$(date +%Y%m%d_%H%M)
for r in $(seq $((SEED0 + 1)) $((SEED0 + REPEAT))); do
    seats=24; [ $((r % 2)) -eq 0 ] && seats=35
    for m in 0 1; do
        [ $m -eq 1 ] && { [ "${MIRROR:-0}" = 1 ] || break; seats=$([ $seats = 24 ] && echo 35 || echo 24); }
        ini="$BASE,teams=ffa,ai_cheat=0,ai_ignore=1,headless=1,turbo=1,quit_minutes=$MIN,game_seed=$r,setup=tests/scenarios/$SCEN.eeai"
        ini="$ini,variant_players=$seats,$B"
        job -Save newgame.ees -TimeoutSec $((MIN * 12 + 120)) -Tag "_h2h${STAMP}_${NAME}_${SCEN}-r$r$([ $m -eq 1 ] && echo m)" -IniExtra "$ini"
    done
done
jobs_wait
local/venv/Scripts/python.exe scripts/results/h2h.py local/work/autotest_*_h2h${STAMP}_*.csv*
echo H2H DONE $STAMP

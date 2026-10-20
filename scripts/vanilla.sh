#!/bin/sh
# The mod's AI against the game's own AI (docs/testing.md "Against the base-game AI"): 1v1 random maps, one AI plays
# the shipped config, the other is a vanilla player (eeaimod.c is_vanilla - nothing of the mod acts for it) with the
# game's difficulty and Cheat. Every seed twice, the seats swapped (vanilla P3 / mod P2, then vanilla P2 / mod P3).
# usage: sh scripts/vanilla.sh <minutes> "<base ini keys>" "<vanilla keys>"
#   e.g. sh scripts/vanilla.sh 60 "random_map=Continental,map_size=0,seed=101,start_epoch=0,difficulty=hard" "vanilla_cheat=1"
#   REPEAT=n seeds (game_seed SEED0+1..SEED0+n, default 10), NAME=label, JOBS/SLOT0 (scripts/jobs.sh), DLL, CONFIG.
#   Team games: AIS=4 TEAMS=pairs SEATS="45 23" (the vanilla seats per run: P4+P5 against the mod's P2+P3, then swapped).
#   Scored by scripts/vanilla.py work/autotest_*_van<stamp>_*.csv
cd "$(dirname "$0")/.."
. scripts/jobs.sh
MIN=$1; BASE=$2; V=$3
REPEAT=${REPEAT:-10}; NAME=${NAME:-van}; SEED0=${SEED0:-0}
STAMP=$(date +%Y%m%d_%H%M%S)
for r in $(seq $((SEED0 + 1)) $((SEED0 + REPEAT))); do
    for seat in ${SEATS:-3 2}; do
        ini="ai_count=${AIS:-2},$BASE,teams=${TEAMS:-ffa},ai_cheat=0,ai_ignore=1,headless=1,turbo=1,quit_minutes=$MIN,game_seed=$r"
        ini="$ini,vanilla_players=$seat"
        [ -n "$V" ] && ini="$ini,$V"
        job -Save newgame.ees -TimeoutSec $((MIN * 12 + 180)) -Tag "_van${STAMP}_${NAME}-r${r}v${seat}" -IniExtra "$ini"
    done
done
jobs_wait
venv/Scripts/python.exe scripts/vanilla.py work/autotest_*_van${STAMP}_${NAME}-*.csv
echo VANILLA DONE $STAMP

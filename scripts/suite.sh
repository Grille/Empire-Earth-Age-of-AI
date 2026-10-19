#!/bin/sh
# Run the random-map test suite for one or more AI variants, then score them.
# usage: sh scripts/suite.sh <minutes> <variant>... where variant = name:ini,keys  e.g. base:  safe:safe_sites=1
# REPEAT=n runs every map n times with game_seed 1..n - the same games for every variant (paired A/B).
# Every map runs with Cheat off and Ignorieren on for all AIs; same seeds for every variant -> fair A/B.
# JOBS=n: n games at a time (scripts/jobs.sh).
cd "$(dirname "$0")/.."
. scripts/jobs.sh
MIN=$1; shift
EPOCHS=${EPOCHS-",epoch_min=15,epoch_max=20"}   # test epoch timer; EPOCHS="" uses the game setting
MAPS="Continental:0:3:101:ffa Plains:0:2:202:ffa Highlands:1:4:303:ffa Mediterranean:0:3:404:pairs"
sed -i "s/^quit_minutes=.*/quit_minutes=$MIN/" autotest.ini
STAMP=$(date +%Y%m%d_%H%M)
REPEAT=${REPEAT:-1}
for v in "$@"; do
    name=${v%%:*}; extra=${v#*:}
    for r in $(seq 1 $REPEAT); do
    for m in $MAPS; do
        IFS=: read map size ais seed teams <<EOF2
$m
EOF2
        tag="_suite${STAMP}_${name}_${map}"
        [ "$REPEAT" -gt 1 ] && tag="$tag-r$r"
        ini="random_map=$map,map_size=$size,ai_count=$ais,seed=$seed,teams=$teams,start_epoch=0,ai_cheat=0,ai_ignore=1,headless=1,turbo=1,game_seed=$r${EPOCHS}"
        [ -n "$extra" ] && ini="$ini,$extra"
        job -Save newgame.ees -TimeoutSec $((MIN * 12 + 120)) -Tag "$tag" -IniExtra "$ini"
    done
    done
done
jobs_wait
venv/Scripts/python.exe scripts/score.py ${QUIET:+-q} work/autotest_*_suite${STAMP}_*.csv
echo SUITE DONE $STAMP

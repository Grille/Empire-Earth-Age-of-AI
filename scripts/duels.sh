#!/bin/sh
# Duels: the stats estimate (dll/stats.c) against real fights. One duel per run on Continental 101 (tiny, start epoch
# 3, ~25 s each): P2's A x na at 56,31 against P3's B x nb at 64,31 (open ground, ~20 tiles from both bases);
# `stats fight` every 3 s (the estimate and both sides' units and hp), `firerate` at the end.
# usage: sh scripts/duels.sh ["A na B nb" ...]   (default: six World War I duels); then
#        python on the logs (work/autotest_*_duel_AxN_BxM.csv): the "stats fight 60 31 14" lines over time
cd "$(dirname "$0")/.."
[ $# -gt 0 ] || set -- "204 10 121 10" "153 5 121 10" "153 5 91 5" "204 10 388 5" "152 5 204 10" "121 10 121 10"
for d in "$@"; do
    set -- $d
    {
        echo "@0.1 spawn 2 $1 56 31 $2"; echo "@0.1 spawn 3 $3 64 31 $4"
        python -c "
t=0.14
while t<1.6: print('@%.2f stats fight 60 31 14 $1 $3'%t); t+=0.05
print('@1.6 firerate')"
    } > scripts/zz_duel.setup
    powershell -NoProfile -ExecutionPolicy Bypass -File scripts/run_test.ps1 -Save newgame.ees -TimeoutSec 300 -IniExtra "random_map=Continental,map_size=0,ai_count=2,seed=101,teams=ffa,start_epoch=3,ai_cheat=0,ai_ignore=1,save_state=0,quit_minutes=2,setup=_aimod/scripts/zz_duel.setup" -Tag "_duel_$1x$2_$3x$4" | tail -1
done
rm -f scripts/zz_duel.setup

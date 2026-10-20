#!/bin/sh
# Duels: the stats estimate (dll/stats.c) against real fights. One duel per run on Continental 101 (tiny, start epoch
# 3, ~25 s each): P2's A x na at 56,31 against P3's B x nb at 64,31 (open ground, ~20 tiles from both bases);
# `stats fight` every 3 s (the estimate and both sides' units and hp), `firerate` at the end.
# usage: [EPOCH=n] sh scripts/duels.sh ["A na B nb [gap [C]]" ...]   (default: six World War I duels; gap: tiles between the
#        sides, default 8; C: player 2 or 3 attacks - its units
#        there get attack orders on the nearest enemy every 12 s (stats engage)); then
#        python on the logs (work/autotest_*_duel_AxN_BxM.csv): the "stats fight 60 31 14" lines over time
cd "$(dirname "$0")/.."
[ $# -gt 0 ] || set -- "204 10 121 10" "153 5 121 10" "153 5 91 5" "204 10 388 5" "152 5 204 10" "121 10 121 10"
for d in "$@"; do
    set -- $d
    gap=${5:-8}; ch=${6:-}
    {   # the duel and its measurements (a script, docs/scripts.md)
        echo "wait_until(0.1min)"
        echo "spawn(2, $1, $((60 - gap / 2)), 31, $2)"
        echo "spawn(3, $3, $((60 + gap / 2)), 31, $4)"
        echo "wait_until(0.13min)"
        echo "stats(\"types\", 2, $1, $(($1 + 1)))"
        echo "stats(\"types\", 3, $3, $(($3 + 1)))"
        echo "every(0.05min, 0.14min, 2.59min, (run) => stats(\"fight\", 60, 31, $((gap / 2 + 10)), $1, $3))"
        echo "wait_until(2.6min)"
        echo "firerate()"
    } > scripts/zz_duel.eeai
    setup="scripts/zz_duel.eeai"
    if [ -n "$ch" ]; then   # the attacker's orders: a script of their own beside it (setup=a;b)
        x=$((ch == 2 ? 60 - gap / 2 : 60 + gap / 2))
        {
            echo "wait_until(0.12min)"
            echo "stats(\"engage\", $ch, $x, 31, $((gap / 2 + 6)))"
            echo "every(0.2min, 0.3min, 1.9min, (run) => stats(\"engage\", $ch, $x, 31, $((gap / 2 + 6))))"
        } > scripts/zz_duel_engage.eeai
        setup="scripts/zz_duel_engage.eeai;$setup"
    fi
    powershell -NoProfile -ExecutionPolicy Bypass -File scripts/run_test.ps1 -Save newgame.ees -TimeoutSec 300 -IniExtra "random_map=Continental,map_size=0,ai_count=2,seed=101,teams=ffa,start_epoch=${EPOCH:-3},ai_cheat=0,ai_ignore=1,save_state=0,quit_minutes=3,setup=$setup" -Tag "_duel_$1x$2_$3x$4${5:+_gap$5}${6:+_c$6}" | tail -1
done
rm -f scripts/zz_duel.eeai scripts/zz_duel_engage.eeai

#!/bin/sh
# Ablations against the base-game AI (docs/testing.md "Against the base-game AI"): every variant of a list x cheat 0/1 x
# Continental e0 / Small Islands e5 through scripts/vanilla.sh (1v1, hard, 60 min, seeds x both seats).
# usage: sh scripts/eco_ablate.sh <variants file>      lines: name|[autotest] keys (comma-separated; empty = shipped)
#   REPEAT=n seeds (default 10), JOBS/SLOT0/DLL/CONFIG as for vanilla.sh. Logs tagged ab-<name>-c<cheat>-<map>.
#   Scored by scripts/eco_ablate.py "work/autotest_*_ab-*.csv" (paired with the variant "base") and
#   scripts/eco_timeline.py (timelines, citizen deaths).
cd "$(dirname "$0")/.."
export REPEAT=${REPEAT:-10}
while IFS='|' read -r name keys; do
    case "$name" in ''|\#*) continue;; esac
    for ch in 0 1; do
        for map in cont si5; do
            case $map in
                cont) base="random_map=Continental,map_size=0,seed=101,start_epoch=0,difficulty=hard";;
                si5) base="random_map=Small Islands,map_size=0,seed=101,start_epoch=5,difficulty=hard";;
            esac
            v="vanilla_cheat=$ch"; [ -n "$keys" ] && v="$v,$keys"
            echo "== $name c$ch $map ($v)"
            NAME="ab-$name-c$ch-$map" sh scripts/vanilla.sh 60 "$base" "$v" 2>&1 | grep "games:\|mod - vanilla"
        done
    done
done < "$1"
echo ABLATE DONE

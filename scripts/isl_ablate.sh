#!/bin/sh
# Island ablations against the base-game AI on the maps players play (docs/testing.md "Large island maps"): every
# variant of a list x cheat 0/1 x the maps of MAPS through scripts/vanilla.sh (hard, seeds x both seats).
# usage: sh scripts/isl_ablate.sh <variants file>      lines: name|[autotest] keys (comma-separated; empty = shipped)
#   MAPS (default "siL tiH liH"), CHEATS (default "0 1"), REPEAT=n seeds (default 20), JOBS/SLOT0/DLL/CONFIG as for
#   vanilla.sh. Logs tagged ab-<name>-c<cheat>-<map>; scored by scripts/islands.py [-base NAME] "work/autotest_*_ab-<name>-*.csv"
#   (and scripts/vanilla.py). Variant names must be new (the scorers glob the work folder).
# The maps (all Middle Ages starts, map seed 101):
#   siL  Small Islands, Large map, 1v1, 75 min: 4 free islands (5 mines each)
#   tiH  Tournament Islands, Huge, 1v1, 75 min: 5 free islands (4-5 mines)
#   liH  Large Islands, Huge, 1v1, 75 min: 3 free islands (4-5 mines)
#   siH  Small Islands, Huge, 1v1, 75 min: 9 free islands (2-5 mines)
#   siL2 Small Islands, Large, 2v2 (P2+P3 / P4+P5, the vanilla pair swapped), 75 min
#   si5 / ti5 / li5  the old small maps (map_scale small), 1v1, 60 min, unit limit 500
#   coL  Continental, Large map, 1v1, unit limit 1500, Middle Ages, 75 min (a land map at high population)
#   cont Continental e0, Small (eco_ablate.sh's), 1v1, 60 min, unit limit 500
# Unit limit: the game's, shared by every player incl. the idle human - 1500 = 500 a player in 1v1 (the AIs' cap 505),
# 2500 in 2v2.
cd "$(dirname "$0")/.."
export REPEAT=${REPEAT:-20}
MAPS=${MAPS:-siL tiH liH}; CHEATS=${CHEATS:-0 1}
while IFS='|' read -r name keys; do
    case "$name" in ''|\#*) continue;; esac
    for ch in $CHEATS; do
        for map in $MAPS; do
            min=75; ais=2; teams=ffa; seats="3 2"; e="seed=101,start_epoch=5,difficulty=hard"
            case $map in
                siL) base="random_map=Small Islands,map_scale=large,unit_limit=1500,$e";;
                tiH) base="random_map=Tournament Islands,map_scale=huge,unit_limit=1500,$e";;
                liH) base="random_map=Large Islands,map_scale=huge,unit_limit=1500,$e";;
                siH) base="random_map=Small Islands,map_scale=huge,unit_limit=1500,$e";;
                siL2) base="random_map=Small Islands,map_scale=large,unit_limit=2500,$e"; ais=4; teams=pairs; seats="45 23";;
                si5) base="random_map=Small Islands,map_size=0,$e"; min=60;;
                ti5) base="random_map=Tournament Islands,map_size=0,$e"; min=60;;
                li5) base="random_map=Large Islands,map_size=0,$e"; min=60;;
                coL) base="random_map=Continental,map_scale=large,unit_limit=1500,$e";;
                cont) base="random_map=Continental,map_size=0,seed=101,start_epoch=0,difficulty=hard"; min=60;;
                *) echo "unknown map $map"; continue;;
            esac
            v="vanilla_cheat=$ch"; [ -n "$keys" ] && v="$v,$keys"
            echo "== $name c$ch $map ($v)"
            AIS=$ais TEAMS=$teams SEATS="$seats" NAME="ab-$name-c$ch-$map" sh scripts/vanilla.sh $min "$base" "$v" 2>&1 | grep "games:\|mod - vanilla"
        done
    done
done < "$1"
echo ISL_ABLATE DONE

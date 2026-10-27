#!/bin/sh
# The own renderer's picture against a wrapper's (docs/renderer.md, step 4), part 2: scripts/test/render_compare.ps1
# has started the same game in slots A (the wrapper) and A+1 (the own renderer). tests/probes/render_compare.eeai pauses
# both at the same simulation steps; this shoots each at the same camera spots (the start view and four around it),
# resumes, and at the end compares the pairs (scripts/results/shot_diff.py). The shots, the difference images and the
# table: local/work/rc_<stamp>/. Ends "RC DONE <dir>".
# usage: sh scripts/test/render_compare.sh <stamp> [slot, default 27]   STEP=n (default 25) the camera's offset in tiles;
#   REF=<an earlier run's dir>: only the own renderer shot (render_compare.ps1 -OwnOnly), the wrapper's shots from there
cd "$(dirname "$0")/../.."
STAMP=${1:?stamp from render_compare.ps1}
A=${2:-27}; B=$((A + 1))
PY=local/venv/Scripts/python.exe
OUT=local/work/rc_$STAMP
STEP=${STEP:-25}
TIMES="1 4 8 12"

ctl() { SLOT=$1 $PY scripts/test/eectl.py $2 2>&1; }
nap() { timeout "$1" tail -f /dev/null 2>/dev/null; }

# one slot: at each pause the start view and four spots around it, then on
shoot() {
    s=$1; tag=$2
    for t in $TIMES; do
        ms=$((t * 60000)); n=0
        while :; do
            st=$(ctl $s status | head -1)
            now=$(echo "$st" | sed -n 's/.*time_ms=\([0-9]*\).*/\1/p')
            echo "$st" | grep -q "paused=1" && [ "${now:-0}" -ge $ms ] && break
            n=$((n + 1)); [ $n -gt 900 ] && { echo "$tag: no pause at $t min ($st)"; return 1; }
            nap 2
        done
        nap 3   # a few frames drawn paused
        set -- $(ctl $s camera | sed -n 's/^x=\([0-9]*\)[.0-9]* y=\([0-9]*\).*/\1 \2/p' | head -1)
        cx=${1:-0}; cy=${2:-0}; k=0
        for d in "0 0" "-$STEP 0" "$STEP 0" "0 -$STEP" "0 $STEP"; do
            set -- $d
            [ "$1$2" != "00" ] && { ctl $s "camera $((cx + $1)) $((cy + $2))" >/dev/null; nap 2; }
            ctl $s "shot rc_${STAMP}_${t}m_$k" >/dev/null
            cp "local/work/slot$s/ageofai_dumps/rc_${STAMP}_${t}m_$k.bmp" "$OUT/${tag}_${t}m_$k.bmp" 2>/dev/null
            k=$((k + 1))
        done
        ctl $s "camera $cx $cy" >/dev/null
        ctl $s resume >/dev/null
        echo "$tag: $t min shot (camera $cx $cy, game time $now)"
    done
}

nap 15
if [ -n "$REF" ]; then cp "$REF"/wrapper_*.bmp "$OUT/"; else shoot $A wrapper & fi
shoot $B own &
wait
$PY scripts/results/shot_diff.py "$OUT"
echo "RC DONE $OUT"

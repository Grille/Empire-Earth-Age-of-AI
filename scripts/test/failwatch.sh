#!/bin/sh
# Ends at the first game of a batch that did not end well (its line in the batch's output starts with CRASHED, HUNG,
# TIMEOUT or ABORTED - testd.py), printing that line, or with jobs.sh's last line ("jobs: ...") when the batch is over:
# run in the background beside a batch, its end is the news (a session is told when a background command ends; a
# batch of hours told nothing until all its games were done, a hung one included). Lines already there count too.
# usage: sh scripts/test/failwatch.sh <the batch's output file> [seconds between looks, default 5]
out=$1; every=${2:-5}
[ -n "$out" ] || { echo "usage: failwatch.sh <batch output file>"; exit 2; }
while :; do
    if [ -f "$out" ]; then
        bad=$(grep -E '^(CRASHED|HUNG|TIMEOUT|ABORTED)' "$out" | head -1)
        if [ -n "$bad" ]; then echo "a game failed: $bad"; exit 1; fi
        end=$(grep '^jobs: ' "$out" | tail -1)
        if [ -n "$end" ]; then echo "the batch is over - $end"; exit 0; fi
    fi
    sleep "$every"
done

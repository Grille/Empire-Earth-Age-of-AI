#!/bin/sh
# A multiplayer test game: one test slot hosts, HUMANS-1 others join it over the network (dll/test/netgame.c), all on
# this PC - then their sync traces are diffed: does the game stay in sync on every PC?
# usage: sh scripts/test/mp.sh <minutes> ["<ini keys>"]   e.g. sh scripts/test/mp.sh 10 "ai_diplomacy=1"
#   HUMANS=n (default 2) PCs; SLOT=n (default 25) the host's slot, the joiners' n+1 ...; DLL=<path> as run_test;
#   STEPS=n (default 20000) the sync trace's steps; GAME_SEED=n (default 777); DUMP=<from ms>-<to ms> (default 1-1000,
#   the first step is at 34; from 0 is none) the steps whose stock and units the traces list. Ends "MP DONE <stamp>" or
#   "MP FAIL <crash+desync+nosteps> <stamp>" (crash: a PC that didn't end with exit code 0). Every PC plays the same random map:
#   MAP (default "random_map=Continental,map_size=0,ai_count=2,seed=101") - without one the lobby's own (no AIs).
# The joiners start once the host's lobby is open (one that asks before the host listens stays alone). The traces are
# kept as local/work/mp_<stamp>_s<slot>.txt; the PCs' own lines (the messages each hands its server: a joiner's
# differ from the host's) are left out of the diff, which compares the steps by game time (a PC waiting for a turn
# writes its last step again) that both traces have (the PCs quit a step apart).
cd "$(dirname "$0")/../.."
MIN=${1:?minutes}; KEYS=${2:-}
HUMANS=${HUMANS:-2}; H=${SLOT:-25}; STEPS=${STEPS:-20000}
STAMP=$(date +%Y%m%d_%H%M%S)
MAP=${MAP:-random_map=Continental,map_size=0,ai_count=2,seed=101}
# sync_dump: the first second's stock and units always - a difference at the very first steps was seen twice and never
# again on a rerun, so its details have to be in the trace that shows it
base="$MAP,quit_minutes=$MIN,game_seed=${GAME_SEED:-777},sync_trace=$STEPS,sync_dump=${DUMP:-1-1000},mp_humans=$HUMANS"
[ -n "$KEYS" ] && base="$base,$KEYS"
run() { powershell -NoProfile -ExecutionPolicy Bypass -File scripts/test/run_test.ps1 -TimeoutSec $((MIN * 12 + 240)) "$@" | tail -1; }
log=local/work/slot$H/ageofai_log.csv
mark=local/work/mp_${STAMP}.start; : > "$mark"   # the host's log of this run: newer than this
# a PC's result line (run_test's last); one that didn't end with exit code 0 also its game's exception line, from the
# log run_test has packed by then - a crash says where without a rerun
report() {
    cat "$1"
    grep -q 'exit code 0,' "$1" && return 0
    f=$(sed -n 's/.*log: //p' "$1" | tr '\\' '/')
    [ -f "$f" ] && zcat "$f" | grep -a '^# exception' | cut -c1-400 | head -2
    return 1
}
run -Slot $H -Tag "_mp${STAMP}_host" -IniExtra "mp=host,$base" > "$mark.r0" &
host=$!
t=0
until [ -n "$(find "$log" -newer "$mark" 2>/dev/null)" ] && grep -qa '^# mp: hosting' "$log"; do
    sleep 2; t=$((t + 2))
    if ! kill -0 $host 2>/dev/null || [ $t -ge 180 ]; then
        wait; report "$mark.r0"
        echo "MP FAIL crash: the host's lobby didn't open ($t s)"; rm -f "$mark" "$mark".r*; exit 1
    fi
done
sleep 2
for i in $(seq 1 $((HUMANS - 1))); do
    run -Slot $((H + i)) -Tag "_mp${STAMP}_join$i" -IniExtra "mp=join,mp_host=$H,mp_seat=$i,$base" > "$mark.r$i" &
    sleep 1
done
wait
crash=0
for i in $(seq 0 $((HUMANS - 1))); do report "$mark.r$i" || crash=1; done
rm -f "$mark" "$mark".r*
for i in $(seq 0 $((HUMANS - 1))); do
    cp "local/work/slot$((H + i))/synctrace.txt" "local/work/mp_${STAMP}_s$((H + i)).txt" 2>/dev/null
done
local/venv/Scripts/python.exe - "$STAMP" $crash $(seq $H $((H + HUMANS - 1))) <<'EOF'
import sys
stamp, crash, slots = sys.argv[1], sys.argv[2] == '1', sys.argv[3:]
def steps(s):
    try:
        lines = open('local/work/mp_%s_s%s.txt' % (stamp, s)).read().splitlines()
    except OSError:
        return None
    # the step lines by game ms: a PC waiting for the next turn writes its last step again (as often as it waits - each
    # PC its own count); the start message's line names a value from before the start, each PC's own. The sync_dump
    # lines ("  p ...", "  u ...") belong to the step above them.
    by_ms, dump, ms = {}, {}, None
    for l in lines:
        if l[:1].isdigit():
            ms = int(l.split()[1])
            if ms in by_ms:
                ms = None   # a repeat: its dump is the first one's again
            else:
                by_ms[ms] = l
                dump[ms] = []
        elif l.startswith('  ') and ms is not None:
            dump[ms].append(l)
    return by_ms, dump
t = {s: steps(s) for s in slots}
if not t[slots[0]]:
    print('MP: no trace from the host (slot %s)' % slots[0]); print('MP FAIL crash %s' % stamp); sys.exit(1)
host, hdump = t[slots[0]]
bad = ['crash'] if crash else []
for s in slots[1:]:
    if not t[s]:
        print('MP: no trace from slot %s' % s); bad.append('crash'); continue
    j, jdump = t[s]
    common = sorted(set(host) & set(j))
    d = next((ms for ms in common if host[ms] != j[ms]), None)
    if not common:
        bad.append('nosteps')
        print('MP: slot %s and the host have no step in common' % s)
    elif d is None:
        print('MP: slot %s in sync with the host for %d steps (to game ms %d)' % (s, len(common), common[-1] if common else 0))
    else:
        bad.append('desync')
        print('MP: slot %s OUT OF SYNC at game ms %d:\n  host %s\n  join %s' % (s, d, host[d], j[d]))
        hd, jd = hdump[d], jdump[d]
        if not hd and not jd:
            print('  (no dump there: rerun with DUMP=<from ms>-<to ms> around it)')
        diff = [(a, b) for a, b in zip(hd, jd) if a != b]
        for a, b in diff[:5]:
            print('  host%s\n  join%s' % (a, b))
        if len(hd) != len(jd):
            print('  host dumps %d lines, the joiner %d' % (len(hd), len(jd)))
print('MP FAIL %s %s' % ('+'.join(sorted(set(bad))), stamp) if bad else 'MP DONE %s' % stamp)
sys.exit(1 if bad else 0)
EOF

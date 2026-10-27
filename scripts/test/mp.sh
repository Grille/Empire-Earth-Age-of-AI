#!/bin/sh
# A multiplayer test game: one test slot hosts, HUMANS-1 others join it over the network (dll/test/netgame.c), all on
# this PC - then their sync traces are diffed: does the game stay in sync on every PC?
# usage: sh scripts/test/mp.sh <minutes> ["<ini keys>"]   e.g. sh scripts/test/mp.sh 10 "ai_diplomacy=1"
#   HUMANS=n (default 2) PCs; SLOT=n (default 25) the host's slot, the joiners' n+1 ...; DLL=<path> as run_test;
#   STEPS=n (default 20000) the sync trace's steps; GAME_SEED=n (default 777). Every PC plays the same random map:
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
base="$MAP,quit_minutes=$MIN,game_seed=${GAME_SEED:-777},sync_trace=$STEPS,mp_humans=$HUMANS"
[ -n "$KEYS" ] && base="$base,$KEYS"
run() { powershell -NoProfile -ExecutionPolicy Bypass -File scripts/test/run_test.ps1 -TimeoutSec $((MIN * 12 + 240)) "$@" | tail -1; }
log=local/work/slot$H/aimod_econ.csv
mark=local/work/mp_${STAMP}.start; : > "$mark"   # the host's log of this run: newer than this
run -Slot $H -Tag "_mp${STAMP}_host" -IniExtra "mp=host,$base" &
t=0
until [ -n "$(find "$log" -newer "$mark" 2>/dev/null)" ] && grep -qa '^# mp: hosting' "$log"; do
    sleep 2; t=$((t + 2))
    [ $t -ge 180 ] && { echo "the host's lobby didn't open in 180 s"; wait; exit 1; }
done
sleep 2
for i in $(seq 1 $((HUMANS - 1))); do
    run -Slot $((H + i)) -Tag "_mp${STAMP}_join$i" -IniExtra "mp=join,mp_host=$H,mp_seat=$i,$base" &
    sleep 1
done
wait
rm -f "$mark"
for i in $(seq 0 $((HUMANS - 1))); do
    cp "local/work/slot$((H + i))/synctrace.txt" "local/work/mp_${STAMP}_s$((H + i)).txt" 2>/dev/null
done
local/venv/Scripts/python.exe - "$STAMP" $(seq $H $((H + HUMANS - 1))) <<'EOF'
import sys
stamp, slots = sys.argv[1], sys.argv[2:]
def steps(s):
    try:
        lines = open('local/work/mp_%s_s%s.txt' % (stamp, s)).read().splitlines()
    except OSError:
        return None
    # the step lines by game ms: a PC waiting for the next turn writes its last step again (as often as it waits - each
    # PC its own count); the start message's line names a value from before the start, each PC's own
    by_ms = {}
    for l in lines:
        if l[:1].isdigit():
            by_ms.setdefault(int(l.split()[1]), l)
    return by_ms
t = {s: steps(s) for s in slots}
host = t[slots[0]]
if not host:
    print('MP: no trace from the host (slot %s)' % slots[0]); sys.exit(1)
bad = 0
for s in slots[1:]:
    j = t[s]
    if not j:
        print('MP: no trace from slot %s' % s); bad = 1; continue
    common = sorted(set(host) & set(j))
    d = next((ms for ms in common if host[ms] != j[ms]), None)
    if d is None:
        print('MP: slot %s in sync with the host for %d steps (to game ms %d)' % (s, len(common), common[-1] if common else 0))
    else:
        bad = 1
        print('MP: slot %s OUT OF SYNC at game ms %d:\n  host %s\n  join %s' % (s, d, host[d], j[d]))
print('MP DONE %s' % stamp)
sys.exit(bad)
EOF

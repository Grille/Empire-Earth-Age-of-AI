# Parallel test games for the scripts that run many (suite.sh, scen.sh, h2h.sh, duels.sh): source this file, call
# `job <run_test.ps1 arguments>` for each game and `jobs_wait` before scoring.
# JOBS=n runs up to n games at a time, each in its own test slot 1..n (run_test.ps1 -Slot: own test ini, log, pipe,
# network ports - docs/testing.md, Parallel runs); the default JOBS=1 runs them one after the other in slot 0, as
# before. Each finished game prints run_test's last line. SLOT0=k: slots k+1..k+n instead (two sessions testing at
# once, each in its own slots: SLOT0=20 JOBS=6 plays in slots 21..26).
JOBS=${JOBS:-1}; SLOT0=${SLOT0:-0}
job() {
    if [ "$JOBS" -le 1 ]; then
        powershell -NoProfile -ExecutionPolicy Bypass -File scripts/run_test.ps1 "$@" | tail -1
        return
    fi
    while :; do
        for s in $(seq $((SLOT0 + 1)) $((SLOT0 + JOBS))); do
            pid=$(cat "work/.job$s" 2>/dev/null)
            if [ -z "$pid" ] || ! kill -0 "$pid" 2>/dev/null; then
                (SLOT=$s powershell -NoProfile -ExecutionPolicy Bypass -File scripts/run_test.ps1 "$@" | tail -1) &
                echo $! > "work/.job$s"
                sleep 1   # the next game starts a moment later: not all loading their data at once
                return
            fi
        done
        sleep 1
    done
}
jobs_wait() {
    wait
    for s in $(seq $((SLOT0 + 1)) $((SLOT0 + JOBS))); do rm -f "work/.job$s"; done   # its own slots only
}

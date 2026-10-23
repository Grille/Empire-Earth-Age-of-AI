# Parallel test games for the scripts that run many (suite.sh, scen.sh, h2h.sh, duels.sh): source this file, call
# `job <run_test.ps1 arguments>` for each game and `jobs_wait` before scoring.
# JOBS=n runs up to n games at a time, each in a test slot of its own (run_test.ps1 -Slots: own test ini, log, pipe,
# network ports - docs/testing.md, Parallel runs); the default JOBS=1 runs them one after the other in slot 0, as
# before. Each finished game prints run_test's last line. The slot: the first free one of SLOTS (default 1-30) -
# run_test takes it (one step, nobody else can get it) and waits while all are taken, so sessions testing at once
# never collide and never need slots of their own.
JOBS=${JOBS:-1}
job() {
    if [ "$JOBS" -le 1 ]; then
        powershell -NoProfile -ExecutionPolicy Bypass -File scripts/test/run_test.ps1 "$@" | tail -1
        return
    fi
    while [ "$(jobs -pr | wc -l)" -ge "$JOBS" ]; do wait -n; done   # this shell's own games
    (SLOTS=${SLOTS:-1-30} powershell -NoProfile -ExecutionPolicy Bypass -File scripts/test/run_test.ps1 "$@" | tail -1) &
    sleep 1   # the next game starts a moment later: not all loading their data at once
}
jobs_wait() { wait; }

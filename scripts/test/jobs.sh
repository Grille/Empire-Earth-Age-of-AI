# Parallel test games for the scripts that run many (suite.sh, scen.sh, h2h.sh, duels.sh): source this file, call
# `job <run_test.ps1 arguments>` for each game and `jobs_wait` before scoring.
# JOBS=n: this batch keeps up to n games in run_test's queue at a time (default 1, one after the other); when they
# play and how many in all is the queue's. A batch that put all its games in at once would make everybody else's wait
# for the whole of it - n at a time, sessions' games take turns. Each finished game prints run_test's last line.
JOBS=${JOBS:-1}
job() {
    if [ "$JOBS" -le 1 ]; then
        powershell -NoProfile -ExecutionPolicy Bypass -File scripts/test/run_test.ps1 "$@" | tail -1
        return
    fi
    while [ "$(jobs -pr | wc -l)" -ge "$JOBS" ]; do wait -n; done   # this shell's own games
    (powershell -NoProfile -ExecutionPolicy Bypass -File scripts/test/run_test.ps1 "$@" | tail -1) &
    sleep 1   # the next game starts a moment later: not all loading their data at once
}
jobs_wait() { wait; }

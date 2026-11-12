# Parallel test games for the scripts that run many (suite.sh, scen.sh, h2h.sh, duels.sh): source this file, call
# `job <run_test.py arguments>` for each game and `jobs_wait` before scoring.
# JOBS=n: this batch keeps up to n games in the test server's queue at a time (default 12; 1: one after the other, each
# printed before the next starts); when they play and how many in all is the server's (testd.py - sessions take turns
# there, whatever n is). n only bounds the waiting clients. Each finished game prints run_test.py's last line - one that
# did not end well starts with CRASHED, HUNG, TIMEOUT or ABORTED (testd.py: a hung game is caught within minutes, its
# threads in its log); jobs_wait repeats those at the end ("jobs: n of m games failed") and returns 1 when any did.
JOBS=${JOBS:-12}
JOBS_OUT=$(mktemp 2>/dev/null || echo "${TMPDIR:-/tmp}/jobs_$$")
: > "$JOBS_OUT"
job() {
    if [ "$JOBS" -le 1 ]; then
        local/venv/Scripts/python.exe scripts/test/run_test.py -Quiet "$@" | tee -a "$JOBS_OUT"
        return
    fi
    while [ "$(jobs -pr | wc -l)" -ge "$JOBS" ]; do wait -n; done   # this shell's own games
    local/venv/Scripts/python.exe scripts/test/run_test.py -Quiet "$@" | tee -a "$JOBS_OUT" &
}
jobs_wait() {
    wait
    n=$(grep -c 'finished in\|^error' "$JOBS_OUT")
    bad=$(grep -cE '^(CRASHED|HUNG|TIMEOUT|ABORTED)' "$JOBS_OUT")
    if [ "$bad" -gt 0 ]; then
        echo "jobs: $bad of $n games failed:"
        grep -E '^(CRASHED|HUNG|TIMEOUT|ABORTED)' "$JOBS_OUT"
        rm -f "$JOBS_OUT"
        return 1
    fi
    echo "jobs: all $n games ended well"
    rm -f "$JOBS_OUT"
}

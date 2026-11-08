# Parallel test games for the scripts that run many (suite.sh, scen.sh, h2h.sh, duels.sh): source this file, call
# `job <run_test.py arguments>` for each game and `jobs_wait` before scoring.
# JOBS=n: this batch keeps up to n games in the test server's queue at a time (default 12; 1: one after the other, each
# printed before the next starts); when they play and how many in all is the server's (testd.py - sessions take turns
# there, whatever n is). n only bounds the waiting clients. Each finished game prints run_test.py's last line.
JOBS=${JOBS:-12}
job() {
    if [ "$JOBS" -le 1 ]; then
        local/venv/Scripts/python.exe scripts/test/run_test.py -Quiet "$@"
        return
    fi
    while [ "$(jobs -pr | wc -l)" -ge "$JOBS" ]; do wait -n; done   # this shell's own games
    local/venv/Scripts/python.exe scripts/test/run_test.py -Quiet "$@" &
}
jobs_wait() { wait; }

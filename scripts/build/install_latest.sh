#!/bin/sh
# Keeps the game folder on the newest commit (the user, 2026-10-23: "my install always gets the newest changes - it is
# for testing, not for bug-free games; a crash is good to know too"). The git post-commit hook (.git/hooks/post-commit,
# local to this machine) starts it in the background after every commit, any session's. It installs at once, games
# running or not (install.sh moves files in use aside: a running game keeps its build until it is started again) -
# it used to wait for the player's game to end, and lost the race when he started one meanwhile (2026-11-04). One
# install at a time: a commit while one runs marks it pending, and the running one installs again afterwards (HEAD
# then, whatever came meanwhile). An install that fails is tried again 3 times, 30 s apart (a build that fails isn't:
# it would fail again). Log: local/work/install.log; local/work/installed.txt the commit installed last
# (install_wait.sh waits for it).
# usage: sh scripts/build/install_latest.sh       (normally the hook's)
cd "$(dirname "$0")/../.." || exit 1
mkdir -p local/work
L=local/work/install.lock
touch local/work/install.pending
if ! mkdir "$L" 2>/dev/null; then   # one runs: it picks the pending one up - unless it died (its pid gone)
    pid=$(cat "$L/pid" 2>/dev/null)
    if [ -n "$pid" ] && kill -0 "$pid" 2>/dev/null; then exit 0; fi
    rm -rf "$L"; mkdir "$L" 2>/dev/null || exit 0
fi
echo $$ > "$L/pid"
players() {   # the player's games (Empire Earth / Art of Conquest started without -loadgame: every test run has it)
    powershell -NoProfile -Command "@(Get-CimInstance Win32_Process | Where-Object { (\$_.Name -eq 'Empire Earth.exe' -or \$_.Name -eq 'EE-AOC.exe') -and \$_.CommandLine -notmatch '-loadgame' }).Count" 2>/dev/null | tr -d '\r'
}
tries=0
while [ -f local/work/install.pending ]; do
    rm -f local/work/install.pending
    echo "$(date '+%F %T') installing $(git rev-parse --short HEAD)" >> local/work/install.log
    sh scripts/build/install.sh >> local/work/install.log 2>&1
    rc=$?
    echo "$(date '+%F %T') install exit $rc" >> local/work/install.log
    if [ $rc = 0 ]; then
        tries=0
        [ "$(players)" != 0 ] && echo "  the player's game is running: it keeps the build it started with until it is started again" >> local/work/install.log
    elif [ $rc != 2 ] && [ $tries -lt 3 ]; then
        tries=$((tries + 1)); echo "  FAILED - again in 30 s (try $tries of 3)" >> local/work/install.log
        sleep 30; touch local/work/install.pending
    else
        echo "  FAILED - $([ $rc = 2 ] && echo "the build failed" || echo "given up"); the next commit tries again" >> local/work/install.log
    fi
done
rm -rf "$L"
[ -f local/work/install.pending ] && exec sh scripts/build/install_latest.sh   # a commit came between the last check and the unlock
exit 0

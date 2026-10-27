#!/bin/sh
# Keeps the game folder on the newest commit (the user, 2026-10-23: "my install always gets the newest changes - it is
# for testing, not for bug-free games; a crash is good to know too"). The git post-commit hook (.git/hooks/post-commit,
# local to this machine) starts it in the background after every commit, any session's. It waits until the player's
# own game has ended - a game started without -loadgame (every test run starts with it) - before install.sh reserves
# the game folder: install.sh alone holds the reservation while it waits, and no test of any session could start for
# as long as the player plays. One install at a time: a commit while one runs marks it pending, and the running one
# installs again afterwards (HEAD then, whatever came meanwhile). ageofai_config.exe open is waited for the same way (a
# running program can't be replaced: it was skipped, and the settings program stayed old). Log: local/work/install.log;
# the hook says at once in the commit's output when the game or the settings program is running and the install waits.
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
players() {   # the player's games (Empire Earth / Art of Conquest started without -loadgame) and the settings program open
    powershell -NoProfile -Command "@(Get-CimInstance Win32_Process | Where-Object { ((\$_.Name -eq 'Empire Earth.exe' -or \$_.Name -eq 'EE-AOC.exe') -and \$_.CommandLine -notmatch '-loadgame') -or \$_.Name -eq 'ageofai_config.exe' }).Count" 2>/dev/null | tr -d '\r'
}
while [ -f local/work/install.pending ]; do
    rm -f local/work/install.pending
    told=
    while [ "$(players)" != 0 ]; do
        [ -z "$told" ] && { echo "$(date '+%F %T') $(git rev-parse --short HEAD): waiting for the player's game (or ageofai_config.exe) to end" >> local/work/install.log; told=1; }
        sleep 20
    done
    echo "$(date '+%F %T') installing $(git rev-parse --short HEAD)" >> local/work/install.log
    sh scripts/build/install.sh >> local/work/install.log 2>&1
    rc=$?
    echo "$(date '+%F %T') install exit $rc" >> local/work/install.log
    [ $rc = 3 ] && touch local/work/install.pending   # the settings program opened meanwhile: again once it is closed
done
rm -rf "$L"
[ -f local/work/install.pending ] && exec sh scripts/build/install_latest.sh   # a commit came between the last check and the unlock
exit 0

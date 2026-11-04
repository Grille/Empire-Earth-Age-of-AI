#!/bin/sh
# Waits until the game folder has the commit given (default HEAD) or a later one - for a session to call before it
# tells the player a change is done (the player, 2026-11-04: he kept testing a build from before the fix). The
# post-commit hook installs every commit in the background (install_latest.sh); this reads local/work/installed.txt.
# Exit 0 installed, 1 not within the time (default 600 s; the install's log: local/work/install.log).
# usage: sh scripts/build/install_wait.sh [commit] [seconds]
cd "$(dirname "$0")/../.." || exit 1
want=$(git rev-parse "${1:-HEAD}") || exit 1
end=$(( $(date +%s) + ${2:-600} ))
while :; do
    have=$(cat local/work/installed.txt 2>/dev/null)
    # installed: that commit or one that contains it (a later commit installed in one go)
    if [ -n "$have" ] && git merge-base --is-ancestor "$want" "$have" 2>/dev/null; then
        echo "installed: $(git log -1 --format='%h %s' "$have" | cut -c1-100)"
        pl=$(powershell -NoProfile -Command "@(Get-CimInstance Win32_Process | Where-Object { (\$_.Name -eq 'Empire Earth.exe' -or \$_.Name -eq 'EE-AOC.exe') -and \$_.CommandLine -notmatch '-loadgame' }).Count" 2>/dev/null | tr -d '\r')
        [ "$pl" != 0 ] && echo "the player's game is running: it has the new build once it is started again"
        exit 0
    fi
    if [ "$(date +%s)" -ge "$end" ]; then
        echo "NOT installed within ${2:-600} s - local/work/install.log:"; tail -8 local/work/install.log; exit 1
    fi
    sleep 5
done

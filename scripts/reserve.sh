#!/bin/sh
# Reserves the game folder for a task that needs it to itself (installing the DLL, a slot-0 run): no test run starts a
# game while it holds the reservation (run_test.ps1 waits before its start), and the task begins once the folder's
# game (Empire Earth.exe or EE-AOC.exe) runs no more - the tests' games end by themselves, the player's game is waited for. So a task can be
# queued without asking anyone to pause. The reservation is a named object of the Windows session, by the game
# folder's name (every copy of the repository shares it), held by scripts/hold.ps1 for as long as this shell keeps
# its stdin open - a task that dies frees it with no stale file left. Another task's reservation is waited for. The
# task's own runs (EEAIMOD_RESERVED) pass it.
# usage: sh scripts/reserve.sh "<what>" <command> [args...]     (prints what it waits for; the command's exit code)
# sourced (install.sh): reserve_begin "<what>" ... reserve_end
. "$(dirname "$0")/game_dir.sh"   # GAME, REPO
reserve_games() { tasklist //FI "IMAGENAME eq $GAME_EXE" 2>/dev/null | grep -ci "${GAME_EXE%.exe}"; }   # the folder's game
reserve_begin() {
    told=
    while :; do
        coproc RESERVE_HOLD { powershell -NoProfile -ExecutionPolicy Bypass -File "$REPO/scripts/hold.ps1" -Reserve "$(cygpath -w "$GAME")"; }
        got=; read -r got <&"${RESERVE_HOLD[0]}"
        [ "${got%?}" = held ] || [ "$got" = held ] && break
        wait "$RESERVE_HOLD_PID" 2>/dev/null
        [ -z "$told" ] && { echo "waiting: the game folder is reserved by another task"; told=1; }
        sleep 5
    done
    trap reserve_end EXIT INT TERM
    export EEAIMOD_RESERVED=1
    told=
    while [ "$(reserve_games)" -gt 0 ]; do
        [ -z "$told" ] && { echo "reserved for $1: waiting for $(reserve_games) running game(s) to end"; told=1; }
        sleep 5
    done
}
reserve_end() {   # the holder's stdin closed: it lets go
    if [ -n "${RESERVE_HOLD[1]}" ]; then
        eval "exec ${RESERVE_HOLD[1]}>&-"
        wait "$RESERVE_HOLD_PID" 2>/dev/null
    fi
    unset EEAIMOD_RESERVED
    trap - EXIT INT TERM
}
if [ "$(basename "$0")" = reserve.sh ]; then
    [ $# -ge 2 ] || { echo 'usage: sh scripts/reserve.sh "<what>" <command> [args...]'; exit 1; }
    what=$1; shift
    reserve_begin "$what"
    "$@"
    rc=$?
    reserve_end
    exit $rc
fi

#!/bin/sh
# Reserves the game folder for a task that needs it to itself (installing the DLL, a slot-0 run): no test run starts a
# game while it holds the reservation (run_test.ps1 waits before its start), and the task begins once the folder's
# game (Empire Earth.exe or EE-AOC.exe) runs no more - the tests' games end by themselves, the player's game is waited for. So a task can be
# queued without asking anyone to pause. The reservation is <game>\eeaimod_reserved.lock - in the game folder, which
# every copy of the repository shares: "<Windows PID> <what>"; one whose process is gone counts as free. Another
# task's reservation is waited for (first come, first served). The task's own runs (EEAIMOD_RESERVED) pass it.
# usage: sh scripts/reserve.sh "<what>" <command> [args...]     (prints what it waits for; the command's exit code)
# sourced (install.sh): reserve_begin "<what>" ... reserve_end
. "$(dirname "$0")/game_dir.sh"   # GAME, REPO
RESERVE_FILE="$GAME/eeaimod_reserved.lock"
reserve_pid() { cat /proc/$$/winpid 2>/dev/null || echo $$; }
reserve_alive() { [ -n "$1" ] && tasklist //FI "PID eq $1" 2>/dev/null | grep -qw "$1"; }
reserve_games() { tasklist //FI "IMAGENAME eq $GAME_EXE" 2>/dev/null | grep -ci "${GAME_EXE%.exe}"; }   # the folder's game
reserve_begin() {
    me=$(reserve_pid)
    told=
    while :; do
        if (set -C; echo "$me $1" > "$RESERVE_FILE") 2>/dev/null; then break; fi
        holder=$(cut -d' ' -f1 "$RESERVE_FILE" 2>/dev/null)
        if [ -n "$holder" ] && ! reserve_alive "$holder"; then rm -f "$RESERVE_FILE"; continue; fi   # its process is gone
        [ -z "$told" ] && { echo "waiting: the game folder is reserved ($(cut -d' ' -f2- "$RESERVE_FILE" 2>/dev/null))"; told=1; }
        sleep 5
    done
    trap reserve_end EXIT INT TERM
    export EEAIMOD_RESERVED="$me"
    told=
    while [ "$(reserve_games)" -gt 0 ]; do
        [ -z "$told" ] && { echo "reserved for $1: waiting for $(reserve_games) running game(s) to end"; told=1; }
        sleep 5
    done
}
reserve_end() {
    [ -f "$RESERVE_FILE" ] && [ "$(cut -d' ' -f1 "$RESERVE_FILE")" = "$(reserve_pid)" ] && rm -f "$RESERVE_FILE"
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

#!/bin/sh
# probe.sh <tag> <run_test args...>: start a run, wait for the AI, dump global settings 0x918780..0x918900, quit.
tag=$1; shift
cd "$(dirname "$0")/../.."
REPO="$(pwd)"; . scripts/game_dir.sh   # GAME: the slot-0 log is in the game folder's aimod\
powershell -NoProfile -ExecutionPolicy Bypass -File scripts/test/run_test.ps1 "$@" -Tag "_$tag" > local/work/probe_$tag.launch.txt 2>&1 &
sleep 8
for i in $(seq 1 60); do grep -q "^[0-9]*,2," "$GAME/aimod/aimod_econ.csv" 2>/dev/null && break; sleep 2; done   # 120 s at most
sleep 3
local/venv/Scripts/python.exe scripts/test/eectl.py peek 918780 384 > local/work/probe_$tag.txt
local/venv/Scripts/python.exe scripts/test/eectl.py status >> local/work/probe_$tag.txt
local/venv/Scripts/python.exe scripts/test/eectl.py quit > /dev/null
wait

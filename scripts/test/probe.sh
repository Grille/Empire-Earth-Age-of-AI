#!/bin/sh
# probe.sh <tag> <run_test args...>: start a run, wait for the AI, dump global settings 0x918780..0x918900, quit.
tag=$1; shift
cd "$(dirname "$0")/../.."
export RUN=probe_$tag   # run_test.ps1 and eectl.py take it from the environment
powershell -NoProfile -ExecutionPolicy Bypass -File scripts/test/run_test.ps1 "$@" -Tag "_$tag" > local/work/probe_$tag.launch.txt 2>&1 &
sleep 8
dir=$(local/venv/Scripts/python.exe scripts/test/eectl.py --folder)   # waits while it is queued
for i in $(seq 1 60); do grep -q "^[0-9]*,2," "$dir/ageofai_log.csv" 2>/dev/null && break; sleep 2; done   # 120 s at most
sleep 3
local/venv/Scripts/python.exe scripts/test/eectl.py peek 918780 384 > local/work/probe_$tag.txt
local/venv/Scripts/python.exe scripts/test/eectl.py status >> local/work/probe_$tag.txt
local/venv/Scripts/python.exe scripts/test/eectl.py quit > /dev/null
wait

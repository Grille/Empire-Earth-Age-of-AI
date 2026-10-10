#!/bin/sh
# probe.sh <tag> <run_test args...>: start a run, wait for the AI, dump global settings 0x918780..0x918900, quit.
tag=$1; shift
cd "$(dirname "$0")/.."
powershell -NoProfile -ExecutionPolicy Bypass -File scripts/run_test.ps1 "$@" -Tag "_$tag" > work/probe_$tag.launch.txt 2>&1 &
sleep 8
timeout 120 sh -c 'until grep -q "^[0-9]*,2," ../aimod_econ.csv 2>/dev/null; do sleep 2; done'
sleep 3
venv/Scripts/python.exe scripts/eectl.py peek 918780 384 > work/probe_$tag.txt
venv/Scripts/python.exe scripts/eectl.py status >> work/probe_$tag.txt
venv/Scripts/python.exe scripts/eectl.py quit > /dev/null
wait

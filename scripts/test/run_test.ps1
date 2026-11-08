# The runner is scripts/test/run_test.py now (the test server, 2026-11-08). This stays for shell scripts that were already
# running then, with the old jobs.sh sourced - to be removed once none is left.
& "$PSScriptRoot\..\..\local\venv\Scripts\python.exe" "$PSScriptRoot\run_test.py" @args
exit $LASTEXITCODE

# Holds a named object of this Windows session for a shell script (scripts/test/reserve.sh) - the shell can't hold one
# itself. Prints "held" and keeps it until its stdin closes (the script ends it, or the script's process is gone),
# or prints "taken" and exits at once when another process holds it already. Creating the object is one step that
# either makes it or finds it (two holders can't both get it), and Windows removes it with the last handle - nothing
# stale is left behind however the holder ends.
# usage: powershell -File scripts/test/hold.ps1 -Reserve <game folder>   (the name run_test.ps1's Wait-Reservation checks)
param([Parameter(Mandatory)] [string]$Reserve)
$name = "Local\eeaimod_reserved_" + ((Resolve-Path $Reserve).Path.TrimEnd('\').ToLower() -replace '\\', '/')
$made = $false
$e = [System.Threading.EventWaitHandle]::new($false, [System.Threading.EventResetMode]::ManualReset, $name, [ref]$made)
if (-not $made) { "taken"; exit 2 }
"held"
[void][Console]::In.ReadToEnd()
$e.Dispose()

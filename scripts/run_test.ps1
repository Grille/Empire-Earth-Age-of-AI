# Launches Empire Earth on a saved game (Data\Saved Games\<Save>) in eeaimod autotest mode, waits for
# the game to exit (or TimeoutSec), then archives the log to _aimod\work\.
# Test settings come from _aimod\autotest.ini, copied next to the game only for the run, so normal play
# is never sped up or auto-quit. -Mute (default on) zeroes the game's sound/music volume in the
# registry for the run and restores it afterwards (a backup file covers interrupted runs).
# -Scenario <name> (without .scn) starts Data\Scenarios\<name>.scn through the game's real "play scenario"
# path: the file is copied to Saved Games\<name>.ees for -loadgame, and the DLL (scenario_mode=1) switches
# the game start over to Scenarios\<name>.scn.
param([string]$Save = "aitest.ees", [int]$TimeoutSec = 900, [string]$Tag = "", [bool]$Mute = $true,
      [string]$Scenario = "", [string[]]$IniExtra = @())
$g = (Resolve-Path "$PSScriptRoot\..\..").Path   # game folder; the repository lives in <game>\_aimod
if ($Scenario) {
    Copy-Item "$g\Data\Scenarios\$Scenario.scn" "$g\Data\Saved Games\$Scenario.ees" -Force
    $Save = "$Scenario.ees"
}
$key = 'HKCU:\Software\SSSI\Empire Earth'
$backup = "$g\_aimod\work\sound_settings_backup.json"
$names = 'Sound Volume', 'Music Volume', 'Music Enabled'

function Restore-Sound {
    if (Test-Path $backup) {
        $b = Get-Content $backup -Raw | ConvertFrom-Json
        foreach ($n in $names) { Set-ItemProperty $key -Name $n -Value ([int]$b.$n) }
        Remove-Item $backup -Confirm:$false
    }
}

if (Get-Process -Name "Empire Earth" -ErrorAction SilentlyContinue) { "game already running - aborting"; exit 1 }
if ($Save -match '[-/]') { "save name must not contain - or /"; exit 1 }
Restore-Sound   # leftover from an interrupted run
if ($Mute) {
    $cur = Get-ItemProperty $key
    $o = @{}; foreach ($n in $names) { $o[$n] = $cur.$n }
    $o | ConvertTo-Json | Set-Content $backup
    foreach ($n in $names) { Set-ItemProperty $key -Name $n -Value 0 }
}
Copy-Item "$g\_aimod\autotest.ini" "$g\aimod_autotest.ini" -Force
if ($Scenario) { Add-Content "$g\aimod_autotest.ini" "scenario_mode=1" }
# (-File passes "a=1,b=2" as one string, so split on commas as well)
foreach ($l in ($IniExtra -join ',').Split(',')) { if ($l) { Add-Content "$g\aimod_autotest.ini" $l } }
try {
    Remove-Item "$g\aimod_econ.csv" -Confirm:$false -ErrorAction SilentlyContinue
    $psi = New-Object System.Diagnostics.ProcessStartInfo
    $psi.FileName = "$g\Empire Earth.exe"; $psi.Arguments = "-loadgame `"$Save`""
    $psi.WorkingDirectory = $g; $psi.UseShellExecute = $false
    $t0 = Get-Date
    $p = [System.Diagnostics.Process]::Start($psi)
    if (-not $p.WaitForExit($TimeoutSec * 1000)) { $p.Kill(); $p.WaitForExit(); "TIMEOUT after $TimeoutSec s - killed" }
    $secs = [int]((Get-Date) - $t0).TotalSeconds
} finally {
    Restore-Sound
    Remove-Item "$g\aimod_autotest.ini" -Confirm:$false -ErrorAction SilentlyContinue
}
$stamp = Get-Date -Format "yyyyMMdd_HHmmss"
$dest = "$g\_aimod\work\autotest_${stamp}${Tag}.csv"
if (Test-Path "$g\aimod_econ.csv") { Move-Item "$g\aimod_econ.csv" $dest }
"finished in $secs s real time, exit code $($p.ExitCode), log: $dest"

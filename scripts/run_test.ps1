# Launches Empire Earth on a saved game (Data\Saved Games\<Save>) in eeaimod autotest mode, waits for
# the game to exit (or TimeoutSec), then archives the log to _aimod\work\.
# Test settings come from _aimod\autotest.ini, copied next to the game only for the run, so normal play
# is never sped up or auto-quit. -Mute (default on) zeroes the game's sound/music volume in the
# registry for the run and restores it afterwards (a backup file covers interrupted runs).
# -Scenario <name> (without .scn) starts Data\Scenarios\<name>.scn through the game's real "play scenario"
# path: the file is copied to Saved Games\<name>.ees for -loadgame, and the DLL (scenario_mode=1) switches
# the game start over to Scenarios\<name>.scn.
param([string]$Save = "aitest.ees", [int]$TimeoutSec = 900, [string]$Tag = "", [bool]$Mute = $true,
      [string]$Scenario = "", [string[]]$IniExtra = @(), [string]$Exe = "Empire Earth.exe")
$g = (Resolve-Path "$PSScriptRoot\..\..").Path   # game folder; the repository lives in <game>\_aimod
if ($Scenario) {
    Copy-Item "$g\Data\Scenarios\$Scenario.scn" "$g\Data\Saved Games\$Scenario.ees" -Force
    $Save = "$Scenario.ees"
}
$key = 'HKCU:\Software\SSSI\Empire Earth'
$backup = "$g\_aimod\work\sound_settings_backup.json"
$names = 'Sound Volume', 'Music Volume', 'Music Enabled'

# $userCopy keeps the last settings that weren't muted, so a backup taken in a muted state (overlapping runs)
# can't lose the player's real volumes.
$userCopy = "$g\_aimod\work\sound_settings_user.json"
function Restore-Sound {
    if (Test-Path $backup) {
        $b = Get-Content $backup -Raw | ConvertFrom-Json
        if (([int]$b.'Sound Volume' + [int]$b.'Music Volume') -eq 0 -and (Test-Path $userCopy)) {
            $b = Get-Content $userCopy -Raw | ConvertFrom-Json
        }
        foreach ($n in $names) { Set-ItemProperty $key -Name $n -Value ([int]$b.$n) }
        Remove-Item $backup -Confirm:$false
    }
}

if ($Save -match '[-/]') { "save name must not contain - or /"; exit 1 }
# One run at a time: the lock holds the owner's PID; only the owner restores sound and removes the test ini,
# so a run that was interrupted (or is still waiting for its game) can't clean up under a newer run.
$lock = "$g\_aimod\work\run_test.lock"
New-Item -ItemType Directory -Force "$g\_aimod\work" | Out-Null
if (Test-Path $lock) {
    $owner = [int](Get-Content $lock -Raw -ErrorAction SilentlyContinue)
    if ($owner -and $owner -ne $PID -and (Get-Process -Id $owner -ErrorAction SilentlyContinue)) {
        "another test run (PID $owner) is active - aborting"; exit 1
    }
}
Set-Content $lock $PID
if (Get-Process -Name "Empire Earth" -ErrorAction SilentlyContinue) {
    Remove-Item $lock -Confirm:$false; "game already running - aborting"; exit 1
}
Restore-Sound   # leftover from an interrupted run
if ($Mute) {
    $cur = Get-ItemProperty $key
    $o = @{}; foreach ($n in $names) { $o[$n] = $cur.$n }
    $o | ConvertTo-Json | Set-Content $backup
    if (([int]$o['Sound Volume'] + [int]$o['Music Volume']) -gt 0) { $o | ConvertTo-Json | Set-Content $userCopy }
    foreach ($n in $names) { Set-ItemProperty $key -Name $n -Value 0 }
}
# test ini = autotest.ini + -IniExtra; an extra key replaces the same key from autotest.ini (the DLL reads the
# first occurrence). (-File passes "a=1,b=2" as one string, so split on commas as well)
$extra = @(($IniExtra -join ',').Split(',') | Where-Object { $_ })
if ($Scenario) { $extra += "scenario_mode=1" }
$keys = $extra | ForEach-Object { ($_ -split '=', 2)[0].Trim() }
$lines = Get-Content "$g\_aimod\autotest.ini" | Where-Object { $keys -notcontains (($_ -split '=', 2)[0].Trim()) }
Set-Content "$g\aimod_autotest.ini" (@($lines) + $extra)
try {
    Remove-Item "$g\aimod_econ.csv" -Confirm:$false -ErrorAction SilentlyContinue
    $psi = New-Object System.Diagnostics.ProcessStartInfo
    $psi.FileName = "$g\$Exe";$psi.Arguments = "-loadgame `"$Save`""
    $psi.WorkingDirectory = $g; $psi.UseShellExecute = $false
    $t0 = Get-Date
    $p = [System.Diagnostics.Process]::Start($psi)
    # safety: the DLL logs "# autotest:" at load when it found the test ini; without it the game would run at
    # normal speed with intros and (if the ini vanished) sound - kill it instead
    $ok = $false
    for ($i = 0; $i -lt 40 -and -not $p.HasExited; $i++) {
        Start-Sleep -Milliseconds 250
        if ((Test-Path "$g\aimod_econ.csv") -and (Select-String -Path "$g\aimod_econ.csv" -Pattern '^# autotest:' -Quiet)) { $ok = $true; break }
    }
    if (-not $ok -and -not $p.HasExited) { $p.Kill(); $p.WaitForExit(); "ABORTED: the DLL did not start in autotest mode (test ini missing?)" }
    elseif (-not $p.WaitForExit($TimeoutSec * 1000)) { $p.Kill(); $p.WaitForExit(); "TIMEOUT after $TimeoutSec s - killed" }
    $secs = [int]((Get-Date) - $t0).TotalSeconds
} finally {
    if ((Get-Content $lock -Raw -ErrorAction SilentlyContinue) -match "^$PID") {
        Restore-Sound
        Remove-Item "$g\aimod_autotest.ini" -Confirm:$false -ErrorAction SilentlyContinue
        Remove-Item $lock -Confirm:$false -ErrorAction SilentlyContinue
    }
}
$stamp = Get-Date -Format "yyyyMMdd_HHmmss"
$dest = "$g\_aimod\work\autotest_${stamp}${Tag}.csv"
if (Test-Path "$g\aimod_econ.csv") { Move-Item "$g\aimod_econ.csv" $dest }
"finished in $secs s real time, exit code $($p.ExitCode), log: $dest"

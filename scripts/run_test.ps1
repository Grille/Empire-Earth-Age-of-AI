# Launches Empire Earth on a saved game (Data\Saved Games\<Save>) in eeaimod autotest mode, waits for
# the game to exit (or TimeoutSec), then archives the log to _aimod\work\.
# Test settings come from _aimod\autotest.ini, copied next to the game only for the run, so normal play
# is never sped up or auto-quit. -Mute (default on) zeroes the game's sound/music volume in the
# registry for the run and restores it afterwards (a backup file covers interrupted runs).
# -Scenario <name> (without .scn) starts Data\Scenarios\<name>.scn through the game's real "play scenario"
# path: the file is copied to Saved Games\<name>.ees for -loadgame, and the DLL (scenario_mode=1) switches
# the game start over to Scenarios\<name>.scn.
# -Slot n (1..30; env SLOT for the scripts that call this one): a test game that runs beside others. Its test ini,
# log and dumps are in work\slot<n>\ (the DLL learns it from EEAIMOD_SLOT / EEAIMOD_SLOT_DIR), its control pipe is
# \\.\pipe\eeaimod<n>, the DLL moves its network ports by 2n, and the game's single-instance check is patched at the
# start (docs/testing.md, Parallel runs). Slot 0 (default) is the game folder as before.
# -Dll <path> (env DLL): this run plays with that eeaimod.dll build instead of the installed one - copied to
# work\eeaimod_slot<n>.dll (the source can be rebuilt meanwhile), named in EEAIMOD_DLL; the installed DLL loads it.
param([string]$Save = "aitest.ees", [int]$TimeoutSec = 900, [string]$Tag = "", [bool]$Mute = $true,
      [string]$Scenario = "", [string[]]$IniExtra = @(), [string]$Exe = "Empire Earth.exe",
      [string]$Config = $env:CONFIG, [string]$AiDir = $env:AI_DIR, [int]$Slot = [int]$env:SLOT,
      [string]$Settings = $env:SETTINGS, [string]$Dll = $env:DLL)
$g = (Resolve-Path "$PSScriptRoot\..\..").Path   # game folder; the repository lives in <game>\_aimod
if ($Slot -lt 0 -or $Slot -gt 30) { "slot must be 0..30"; exit 1 }
$sdir = if ($Slot) { "$g\_aimod\work\slot$Slot" } else { $g }   # this run's test ini, log and dumps
if ($Scenario) {
    # beside other slots the file may be open in another game: copy it only when it differs
    $src = "$g\Data\Scenarios\$Scenario.scn"; $dst = "$g\Data\Saved Games\$Scenario.ees"
    if (-not (Test-Path $dst) -or (Get-FileHash $src).Hash -ne (Get-FileHash $dst).Hash) { Copy-Item $src $dst -Force }
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
# The settings the DLL plays with: -Config (or env CONFIG) = an aimod.ini, -AiDir (or env AI_DIR) = the personality
# folder; relative paths from _aimod. Default: the shipped config\aimod.ini and config\aimod_ai - what players get,
# never the player's own ..\aimod.ini (its experiments would bias every A/B). CONFIG=../aimod.ini tests the player's.
if (-not $Config) { $Config = "config\aimod.ini" }
if (-not $AiDir) { $AiDir = "config\aimod_ai" }
if (-not [System.IO.Path]::IsPathRooted($Config)) { $Config = Join-Path "$g\_aimod" $Config }
if (-not [System.IO.Path]::IsPathRooted($AiDir)) { $AiDir = Join-Path "$g\_aimod" $AiDir }
if (-not (Test-Path $Config -PathType Leaf)) { "config not found: $Config"; exit 1 }
if (-not (Test-Path $AiDir -PathType Container)) { "personality folder not found: $AiDir"; exit 1 }
$Config = (Resolve-Path $Config).Path; $AiDir = (Resolve-Path $AiDir).Path
# One run per slot: the slot's lock holds the owner's PID (and, once started, its game's); only the owner removes
# the test ini, so a run that was interrupted (or is still waiting for its game) can't clean up under a newer run.
$lock = if ($Slot) { "$g\_aimod\work\run_test_slot$Slot.lock" } else { "$g\_aimod\work\run_test.lock" }
New-Item -ItemType Directory -Force "$g\_aimod\work" | Out-Null
New-Item -ItemType Directory -Force $sdir | Out-Null
if (Test-Path $lock) {
    $owner = [int]((Get-Content $lock -Raw -ErrorAction SilentlyContinue) -split ' ')[0]
    if ($owner -and $owner -ne $PID -and (Get-Process -Id $owner -ErrorAction SilentlyContinue)) {
        "another test run (PID $owner) is active in slot $Slot - aborting"; exit 1
    }
}
Set-Content $lock $PID
# the other slots' runs still going: "runner game" PIDs from their locks
function Other-Runs {
    Get-ChildItem "$g\_aimod\work" -Filter "run_test*.lock" | Where-Object { $_.FullName -ne $lock } | ForEach-Object {
        $f = (Get-Content $_.FullName -Raw -ErrorAction SilentlyContinue) -split '\s+' | Where-Object { $_ }
        if ($f -and (Get-Process -Id ([int]$f[0]) -ErrorAction SilentlyContinue)) { , @($f | ForEach-Object { [int]$_ }) }
    }
}
# Keep the PC awake while the run lasts (the screen may still turn off): ES_CONTINUOUS | ES_SYSTEM_REQUIRED for this
# thread - Windows drops it when this process exits. Unattended suites were stopped by the PC going to sleep.
try {
    Add-Type -Namespace EEAimod -Name Power -MemberDefinition '[DllImport("kernel32.dll")] public static extern uint SetThreadExecutionState(uint f);' -ErrorAction Stop
    [void][EEAimod.Power]::SetThreadExecutionState([uint32]"0x80000001")
} catch { "keep-awake request failed: $_" }
# a game that is no test slot's - the player's own - is never disturbed (its sound settings, its log)
$others = @(Other-Runs)
$ours = @($others | ForEach-Object { $_ | Select-Object -Skip 1 })
# a game another runner has just started, before its PID is in that runner's lock: its parent is that runner
$runners = @($others | ForEach-Object { $_[0] })
$foreign = @(Get-Process -Name "Empire Earth" -ErrorAction SilentlyContinue | Where-Object { $ours -notcontains $_.Id } | Where-Object {
    $pp = (Get-CimInstance Win32_Process -Filter "ProcessId=$($_.Id)" -ErrorAction SilentlyContinue).ParentProcessId
    -not ($pp -and $runners -contains [int]$pp) })
if ($foreign) {
    Remove-Item $lock -Confirm:$false; "game already running - aborting"; exit 1
}
if (-not $others) { Restore-Sound }   # leftover from an interrupted run (with other slots running: theirs)
# The game's settings. With the mod's dinput.dll in the game folder (dll/eesettings.c) every run reads a copy of
# -Settings (env SETTINGS; default config\test\eesettings.ini: fixed settings, sound off) - not the player's
# eesettings.ini, not the registry - and nothing needs muting. -Mute:$false: the player's own settings.
$proxy = (Test-Path "$g\dinput.dll") -and (Select-String -Path "$g\dinput.dll" -Pattern 'EEAimodSettings' -SimpleMatch -Quiet)
if (-not $Settings) { $Settings = "$g\_aimod\config\test\eesettings.ini" }
elseif (-not [System.IO.Path]::IsPathRooted($Settings)) { $Settings = Join-Path "$g\_aimod" $Settings }
if ($proxy -and $Mute) {
    if (-not (Test-Path $Settings)) { Remove-Item $lock -Confirm:$false; "settings file not found: $Settings"; exit 1 }
    $env:EEAIMOD_SETTINGS = "$g\_aimod\work\settings_slot$Slot.ini"
    Copy-Item $Settings $env:EEAIMOD_SETTINGS -Force
} else { Remove-Item Env:EEAIMOD_SETTINGS -ErrorAction SilentlyContinue }
# this run's own DLL: the installed one (which needs to know EEAIMOD_DLL) loads the copy and stays out of the way
if ($Dll) {
    if (-not [System.IO.Path]::IsPathRooted($Dll)) { $Dll = Join-Path "$g\_aimod" $Dll }
    $why = if (-not (Test-Path $Dll)) { "-Dll not found: $Dll" }
           elseif (-not (Select-String -Path "$g\eeaimod.dll" -Pattern 'EEAIMOD_DLL' -SimpleMatch -Quiet)) {
               "the installed eeaimod.dll predates -Dll (install a current build once: sh dll/build.sh install)" }
    if ($why) { Remove-Item $lock -Confirm:$false; $why; exit 1 }
    $env:EEAIMOD_DLL = "$g\_aimod\work\eeaimod_slot$Slot.dll"
    try { Copy-Item $Dll $env:EEAIMOD_DLL -Force -ErrorAction Stop }
    catch { Remove-Item $lock -Confirm:$false; "can't copy -Dll to $($env:EEAIMOD_DLL): $_"; exit 1 }
} else { Remove-Item Env:EEAIMOD_DLL -ErrorAction SilentlyContinue }
if ($Mute -and -not $proxy -and -not (Test-Path $backup)) {   # muted once for all slots that run at the same time
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
$extra += "config=$Config", "ai_dir=$AiDir"
$keys = $extra | ForEach-Object { ($_ -split '=', 2)[0].Trim() }
$lines = Get-Content "$g\_aimod\autotest.ini" | Where-Object { $keys -notcontains (($_ -split '=', 2)[0].Trim()) }
Set-Content "$sdir\aimod_autotest.ini" (@($lines) + $extra)
# the game started from here sees the slot (and a slot-0 run none, whatever this shell has set)
if ($Slot) { $env:EEAIMOD_SLOT = "$Slot"; $env:EEAIMOD_SLOT_DIR = $sdir }
else { Remove-Item Env:EEAIMOD_SLOT, Env:EEAIMOD_SLOT_DIR -ErrorAction SilentlyContinue }
# headless: the mod's dinput.dll keeps every window of the game from taking the focus or a taskbar button, from the
# process start on (dll/headless_win.c; eeaimod.dll alone only from its load on)
if ($extra -contains "headless=1") { $env:EEAIMOD_HEADLESS = "1" } else { Remove-Item Env:EEAIMOD_HEADLESS -ErrorAction SilentlyContinue }
try {
    # an existing log in the game folder is from normal play (tests archive theirs below): keep it, never delete it;
    # in a slot's folder: a crashed run's
    if (Test-Path "$sdir\aimod_econ.csv") {
        $kept = "$g\_aimod\work\$(if ($Slot) { 'slot' + $Slot + '_left_' } else { 'play_' })$(Get-Date -Format yyyyMMdd_HHmmss).csv"
        Move-Item "$sdir\aimod_econ.csv" $kept -Force
        "kept the previous log: $kept"
    }
    $t0 = Get-Date
    # headless or beside other games: start suspended and patch before the game runs (else a plain start)
    if ($extra -contains "headless=1" -or $Slot -or $others) {
        # headless: start suspended and switch off the loading splash before the game runs - it comes up before
        # the engine loads our DLL, centred on screen, and takes the focus (jne at 0x534400 -> jmp, see display.c)
        Add-Type @"
using System; using System.Runtime.InteropServices;
public static class SuspendedStart {
  [StructLayout(LayoutKind.Sequential, CharSet=CharSet.Unicode)] struct SI { public int cb; public string a, b, c;
    public int x, y, w, h, xc, yc, fill, flags; public short show, r2; public IntPtr r3, i, o, e; }
  [StructLayout(LayoutKind.Sequential)] struct PI { public IntPtr hp, ht; public int pid, tid; }
  [DllImport("kernel32", CharSet=CharSet.Unicode, SetLastError=true)] static extern bool CreateProcessW(string app,
    string cmd, IntPtr pa, IntPtr ta, bool inh, int flags, IntPtr env, string dir, ref SI si, out PI pi);
  [DllImport("kernel32")] static extern bool WriteProcessMemory(IntPtr p, IntPtr a, byte[] b, int n, out IntPtr w);
  [DllImport("kernel32")] static extern bool ReadProcessMemory(IntPtr p, IntPtr a, byte[] b, int n, out IntPtr r);
  [DllImport("kernel32")] static extern bool VirtualProtectEx(IntPtr p, IntPtr a, int n, int prot, out int old);
  [DllImport("kernel32")] static extern int ResumeThread(IntPtr t);
  [DllImport("kernel32")] static extern bool CloseHandle(IntPtr h);
  static bool Patch(IntPtr hp, int at, byte[] want, byte[] put) {   // only while the bytes are the expected ones
    byte[] cur = new byte[want.Length]; IntPtr n; int old;
    if (!ReadProcessMemory(hp, new IntPtr(at), cur, cur.Length, out n)) return false;
    for (int i = 0; i < want.Length; i++) if (cur[i] != want[i]) return false;
    return VirtualProtectEx(hp, new IntPtr(at), put.Length, 0x40, out old) && WriteProcessMemory(hp, new IntPtr(at), put, put.Length, out n);
  }
  // splash: the loading splash off (jne at 0x534400 -> jmp); multi: the single-instance check off (its je at 0x535CDA
  // nopped - the mutex "StainlessSteelStudiosPresentsEmpireEarth" is created before the engine loads our DLL)
  public static int Start(string exe, string args, string dir, bool splash, bool multi, out bool splashOk, out bool multiOk) {
    SI si = new SI(); si.cb = Marshal.SizeOf(si); PI pi;
    if (splash) si.flags = 0x80;   // headless: STARTF_FORCEOFFFEEDBACK - no busy cursor while it starts
    if (!CreateProcessW(exe, "\"" + exe + "\" " + args, IntPtr.Zero, IntPtr.Zero, false, 4, IntPtr.Zero, dir, ref si, out pi))
      throw new Exception("CreateProcess failed: " + Marshal.GetLastWin32Error());
    splashOk = splash && Patch(pi.hp, 0x534400, new byte[] { 0x75, 0x2F }, new byte[] { 0xEB });
    multiOk = multi && Patch(pi.hp, 0x535CDA, new byte[] { 0x0F, 0x84, 0xBE, 0x02, 0x00, 0x00 },
                             new byte[] { 0x90, 0x90, 0x90, 0x90, 0x90, 0x90 });
    ResumeThread(pi.ht); CloseHandle(pi.ht); CloseHandle(pi.hp);
    return pi.pid;
  }
}
"@
        $headless = $extra -contains "headless=1"; $multi = [bool]($Slot -or $others)
        $splashOk = $false; $multiOk = $false
        $p = Get-Process -Id ([SuspendedStart]::Start("$g\$Exe", "-loadgame `"$Save`"", $g, $headless, $multi, [ref]$splashOk, [ref]$multiOk))
        $null = $p.Handle   # keep a handle, else ExitCode is lost
        if ($headless -and -not $splashOk) { "warning: loading splash not switched off (unexpected bytes at 0x534400)" }
        if ($multi -and -not $multiOk) { "warning: single-instance check not switched off (unexpected bytes at 0x535CDA)" }
    } else {
        $psi = New-Object System.Diagnostics.ProcessStartInfo
        $psi.FileName = "$g\$Exe";$psi.Arguments = "-loadgame `"$Save`""
        $psi.WorkingDirectory = $g; $psi.UseShellExecute = $false
        $p = [System.Diagnostics.Process]::Start($psi)
    }
    Set-Content $lock "$PID $($p.Id)"   # the game is a test slot's: another run doesn't take it for the player's
    # safety: the DLL logs "# autotest:" at load when it found the test ini; without it the game would run at
    # normal speed with intros and (if the ini vanished) sound - kill it instead
    $ok = $false
    for ($i = 0; $i -lt 40 -and -not $p.HasExited; $i++) {
        Start-Sleep -Milliseconds 250
        if ((Test-Path "$sdir\aimod_econ.csv") -and (Select-String -Path "$sdir\aimod_econ.csv" -Pattern '^# autotest:' -Quiet)) { $ok = $true; break }
    }
    if (-not $ok -and -not $p.HasExited) { $p.Kill(); $p.WaitForExit(); "ABORTED: the DLL did not start in autotest mode (test ini missing?)" }
    elseif (-not $p.WaitForExit($TimeoutSec * 1000)) { $p.Kill(); $p.WaitForExit(); "TIMEOUT after $TimeoutSec s - killed" }
    $secs = [int]((Get-Date) - $t0).TotalSeconds
} finally {
    if ((Get-Content $lock -Raw -ErrorAction SilentlyContinue) -match "^$PID") {
        Remove-Item "$sdir\aimod_autotest.ini" -Confirm:$false -ErrorAction SilentlyContinue
        Remove-Item $lock -Confirm:$false -ErrorAction SilentlyContinue
        if (-not @(Other-Runs)) { Restore-Sound }   # the last slot running restores the player's sound
    }
}
$stamp = Get-Date -Format "yyyyMMdd_HHmmss"
$dest = "$g\_aimod\work\autotest_${stamp}$(if ($Slot) { "_s$Slot" })${Tag}.csv"
if (Test-Path "$sdir\aimod_econ.csv") { Move-Item "$sdir\aimod_econ.csv" $dest }
"finished in $secs s real time, exit code $($p.ExitCode), log: $dest"

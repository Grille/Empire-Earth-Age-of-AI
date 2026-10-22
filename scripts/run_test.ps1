# Launches Empire Earth on a saved game (Data\Saved Games\<Save>) in eeaimod autotest mode, waits for
# the game to exit (or TimeoutSec), then archives the log to the repository's work\.
# The game folder: env EE_GAME_DIR, else the line in the repository's game_dir.txt (git-ignored), else the folder the
# repository sits in (<game>\_aimod) - Empire Earth.exe must be there.
# Test settings come from the repository's autotest.ini, copied next to the game only for the run, so normal play
# is never sped up or auto-quit. -Mute (default on) zeroes the game's sound/music volume in the
# registry for the run and restores it afterwards (a backup file covers interrupted runs).
# -Scenario <name> (without .scn) starts Data\Scenarios\<name>.scn through the game's real "play scenario"
# path: the file is copied to Saved Games\<name>.ees for -loadgame, and the DLL (scenario_mode=1) switches
# the game start over to Scenarios\<name>.scn.
# -Slot n (1..30; env SLOT for the scripts that call this one): a test game that runs beside others. Its test ini,
# log and dumps are in work\slot<n>\ (the DLL learns it from EEAIMOD_SLOT / EEAIMOD_SLOT_DIR), its control pipe is
# \\.\pipe\eeaimod<n>, the DLL moves its network ports by 2n, and the game's single-instance check is patched at the
# start (docs/testing.md, Parallel runs). Slot 0 (default) is the game folder as before. -Slots a-b (env SLOTS): the
# first free slot of that range (scripts/jobs.sh: 1-30), waiting while all are taken.
# -Dll <path> (env DLL): this run plays with that eeaimod.dll build instead of the installed one - copied to
# work\eeaimod_slot<n>.dll (the source can be rebuilt meanwhile), named in EEAIMOD_DLL; the installed DLL loads it.
# Art of Conquest: EE_GAME_DIR=<its folder> (EE-AOC.exe is found there; -Exe names another). The mod plays there in
# test runs only until that build is ready (EEAIMOD_UNREADY, game/api/exe.c); the game needs a save of its own to start
# from (Saved Games\aitest.ees - a random_map start replaces what it holds).
param([string]$Save = "aitest.ees", [int]$TimeoutSec = 900, [string]$Tag = "", [bool]$Mute = $true,
      [string]$Scenario = "", [string[]]$IniExtra = @(), [string]$Exe = "",
      [string]$Config = $env:CONFIG, [string]$AiDir = $env:AI_DIR, [int]$Slot = [int]$env:SLOT,
      [string]$Settings = $env:SETTINGS, [string]$Dll = $env:DLL, [string]$Slots = $env:SLOTS)
$repo = (Resolve-Path "$PSScriptRoot\..").Path
$g = $env:EE_GAME_DIR
if (-not $g -and (Test-Path "$repo\game_dir.txt")) { $g = (Get-Content "$repo\game_dir.txt" | Where-Object { $_.Trim() } | Select-Object -First 1).Trim() }
if (-not $g) { $g = Split-Path $repo -Parent }
if (-not $Exe) { $Exe = if (Test-Path (Join-Path $g "EE-AOC.exe") -PathType Leaf) { "EE-AOC.exe" } else { "Empire Earth.exe" } }
if (-not (Test-Path (Join-Path $g $Exe))) { "no $Exe in $g - name the game folder in EE_GAME_DIR or $repo\game_dir.txt"; exit 1 }
# the build's own: its registry key, and the two patches of a suspended start - the loading splash's jne (raw/hooks.h
# CODE_SPLASH_START_JNE) and the single-instance check's je after its CreateMutexA (both builds' 0x535CA7 / 0x540D73)
$aoc = $Exe -eq "EE-AOC.exe"
$build = if ($aoc) { @{ Key = 'HKCU:\Software\Mad Doc Software\EE-AOC'; Splash = 0x53F2FE; Multi = 0x540DA6;
                        MultiBytes = [byte[]](0x0F, 0x84, 0xC8, 0x03, 0x00, 0x00) } }
         else { @{ Key = 'HKCU:\Software\SSSI\Empire Earth'; Splash = 0x534400; Multi = 0x535CDA;
                   MultiBytes = [byte[]](0x0F, 0x84, 0xBE, 0x02, 0x00, 0x00) } }
$exeName = [System.IO.Path]::GetFileNameWithoutExtension($Exe)   # its process
$g = (Resolve-Path $g).Path.TrimEnd('\')
if ($Slot -lt 0 -or $Slot -gt 30) { "slot must be 0..30"; exit 1 }
$slotFrom = $Slot; $slotTo = $Slot
if ($Slots) {
    if ($Slots -notmatch '^\s*(\d+)\s*(?:-\s*(\d+))?\s*$') { "slots must be n or a-b"; exit 1 }
    $slotFrom = [int]$Matches[1]; $slotTo = if ($Matches[2]) { [int]$Matches[2] } else { $slotFrom }
    if ($slotFrom -lt 1 -or $slotTo -gt 30 -or $slotFrom -gt $slotTo) { "slots must lie in 1..30"; exit 1 }
}
# Who holds what is told by named objects of this Windows session, not by files: creating one is a single step that
# either makes it or finds it there (two runs can't both get it), and it is gone once the last process holding a
# handle ends, however it ended (no stale state, no PID reused by another process).
#   eeaimod_slot<n>      the slot: its runner and (a handle given to it at the start) its game - a game that outlives a
#                        killed runner still keeps its slot, so no second game plays into its test ini, log and pipe
#   eeaimod_runner_<pid> a runner is going;  eeaimod_game_<pid> a test game (held by the game only)
#   eeaimod_reserved_<game folder>  scripts/reserve.sh holds the game folder
function New-Named([string]$name) {   # the new object, or $null when it exists already
    $made = $false
    $e = [System.Threading.EventWaitHandle]::new($false, [System.Threading.EventResetMode]::ManualReset, "Local\$name", [ref]$made)
    if ($made) { return $e }
    $e.Dispose(); $null
}
function Test-Named([string]$name) {
    $e = $null
    if (-not [System.Threading.EventWaitHandle]::TryOpenExisting("Local\$name", [ref]$e)) { return $false }
    $e.Dispose(); $true
}
$runner = New-Named "eeaimod_runner_$PID"
# One run per slot. -Slots a-b (env SLOTS, scripts/jobs.sh): the first free slot of the range, waiting while all are
# taken. Nothing to release by hand: the handles close with this process.
$told = $false
for (;;) {
    for ($s = $slotFrom; $s -le $slotTo -and -not $slotHeld; $s++) {
        $slotHeld = New-Named "eeaimod_slot$s"
        # changeover (2026-10-22, remove once no run of the old run_test is left): its lock file with a live runner
        $old = "$repo\work\run_test$(if ($s) { "_slot$s" }).lock"
        $owner = if (Test-Path $old) { [int]((Get-Content $old -Raw -ErrorAction SilentlyContinue) -split ' ')[0] }
        if ($slotHeld -and $owner -and (Get-Process -Id $owner -ErrorAction SilentlyContinue)) { $slotHeld.Dispose(); $slotHeld = $null }
        if ($slotHeld) { $Slot = $s }
    }
    if ($slotHeld) { break }
    if ($slotFrom -eq $slotTo -and -not $Slots) { "another test run is active in slot $Slot - aborting"; exit 1 }
    if (-not $told) { "waiting: slots $slotFrom..$slotTo are all taken"; $told = $true }
    Start-Sleep -Seconds 2
}
$sdir = if ($Slot) { "$repo\work\slot$Slot" } else { $g }   # this run's test ini, log and dumps
if ($Scenario) {
    # beside other slots the file may be open in another game: copy it only when it differs
    $src = "$g\Data\Scenarios\$Scenario.scn"; $dst = "$g\Data\Saved Games\$Scenario.ees"
    if (-not (Test-Path $dst) -or (Get-FileHash $src).Hash -ne (Get-FileHash $dst).Hash) { Copy-Item $src $dst -Force }
    $Save = "$Scenario.ees"
}
$key = $build.Key
$backup = "$repo\work\sound_settings_backup.json"
$names = 'Sound Volume', 'Music Volume', 'Music Enabled'

# $userCopy keeps the last settings that weren't muted, so a backup taken in a muted state (overlapping runs)
# can't lose the player's real volumes.
$userCopy = "$repo\work\sound_settings_user.json"
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
# folder; relative paths from the repository. Default: the shipped config\aimod.ini and config\aimod_ai - what players get,
# never the player's own aimod.ini in the game folder (its experiments would bias every A/B); CONFIG=<game>\aimod.ini
# tests it.
if (-not $Config) { $Config = "config\aimod.ini" }
if (-not $AiDir) { $AiDir = "config\aimod_ai" }
if (-not [System.IO.Path]::IsPathRooted($Config)) { $Config = Join-Path $repo $Config }
if (-not [System.IO.Path]::IsPathRooted($AiDir)) { $AiDir = Join-Path $repo $AiDir }
if (-not (Test-Path $Config -PathType Leaf)) { "config not found: $Config"; exit 1 }
if (-not (Test-Path $AiDir -PathType Container)) { "personality folder not found: $AiDir"; exit 1 }
$Config = (Resolve-Path $Config).Path; $AiDir = (Resolve-Path $AiDir).Path
New-Item -ItemType Directory -Force "$repo\work" | Out-Null
New-Item -ItemType Directory -Force $sdir | Out-Null
# the other runners still going (their PIDs)
function Other-Runners {
    @(Get-Process -Name powershell, pwsh -ErrorAction SilentlyContinue |
      Where-Object { $_.Id -ne $PID -and (Test-Named "eeaimod_runner_$($_.Id)") } | ForEach-Object { $_.Id })
}
# the game folder reserved for a task that needs it to itself (scripts/reserve.sh: an install, a slot-0 run): no game
# starts until it is done - checked right before the start; the task's own runs (EEAIMOD_RESERVED) pass. The name is
# the folder's, as scripts/hold.ps1 makes it.
function Wait-Reservation {
    if ($env:EEAIMOD_RESERVED) { return }
    $told = $false
    while (Test-Named ("eeaimod_reserved_" + ($g.ToLower() -replace '\\', '/'))) {
        if (-not $told) { "waiting: the game folder is reserved (scripts/reserve.sh)"; $told = $true }
        Start-Sleep -Seconds 5
    }
}
# Keep the PC awake while the run lasts (the screen may still turn off): ES_CONTINUOUS | ES_SYSTEM_REQUIRED for this
# thread - Windows drops it when this process exits. Unattended suites were stopped by the PC going to sleep.
try {
    Add-Type -Namespace EEAimod -Name Power -MemberDefinition '[DllImport("kernel32.dll")] public static extern uint SetThreadExecutionState(uint f);' -ErrorAction Stop
    [void][EEAimod.Power]::SetThreadExecutionState([uint32]"0x80000001")
} catch { "keep-awake request failed: $_" }
# a game that is no test slot's - the player's own - is never disturbed. A slot (1..30) has its own test ini, log,
# dumps and pipe, and with the mod's dinput.dll its own settings: it runs beside the player's game. Slot 0 shares the
# game folder's (and without that dinput.dll every run mutes the sound in the registry): not while the player plays
$proxy = (Test-Path "$g\dinput.dll") -and (Select-String -Path "$g\dinput.dll" -Pattern 'EEAimodSettings' -SimpleMatch -Quiet)
$others = Other-Runners
# a test game holds its eeaimod_game_<pid>; one a runner is just starting may not have it yet: its parent is a runner
$foreign = @(Get-Process -Name $exeName -ErrorAction SilentlyContinue | Where-Object { -not (Test-Named "eeaimod_game_$($_.Id)") } | Where-Object {
    $pp = (Get-CimInstance Win32_Process -Filter "ProcessId=$($_.Id)" -ErrorAction SilentlyContinue).ParentProcessId
    -not ($pp -and (Test-Named "eeaimod_runner_$pp")) })
if ($foreign -and (-not $Slot -or ($Mute -and -not $proxy))) {
    "game already running - aborting$(if (-not $Slot) { ' (slot 0 shares its folder: -Slot n runs beside it)' } else { ' (no mod dinput.dll: the run would mute it)' })"; exit 1
}
if (-not $others) { Restore-Sound }   # leftover from an interrupted run (with other slots running: theirs)
# The game's settings. With the mod's dinput.dll in the game folder (dll/dinput/eesettings.c) every run reads a copy of
# -Settings (env SETTINGS; default config\test\eesettings.ini: fixed settings, sound off) - not the player's
# eesettings.ini, not the registry - and nothing needs muting. -Mute:$false: the player's own settings.
if (-not $Settings) { $Settings = "$repo\config\test\eesettings.ini" }
elseif (-not [System.IO.Path]::IsPathRooted($Settings)) { $Settings = Join-Path $repo $Settings }
if ($proxy -and $Mute) {
    if (-not (Test-Path $Settings)) { "settings file not found: $Settings"; exit 1 }
    $env:EEAIMOD_SETTINGS = "$repo\work\settings_slot$Slot.ini"
    Copy-Item $Settings $env:EEAIMOD_SETTINGS -Force
    # Art of Conquest checks for the 2017 update's install ("needs the original game") by its "Installed From" values -
    # the player's own, not in the shared file: the registry's (both keys; doubled backslashes: the file's escape)
    if ($aoc) {
        $add = @()
        foreach ($k in 'HKCU:\Software\SSSI\Empire Earth', $build.Key) {
            $r = Get-ItemProperty $k -ErrorAction SilentlyContinue
            if (-not $r) { continue }
            $add += "", "[$($k -replace '^HKCU:', 'HKCU')]"
            foreach ($n in 'Installed From Volume', 'Installed From Directory') {
                if ($r.$n) { $add += "$n=`"$($r.$n -replace '\\', '\\' -replace '"', '\"')`"" }
            }
        }
        Add-Content $env:EEAIMOD_SETTINGS $add
    }
} else { Remove-Item Env:EEAIMOD_SETTINGS -ErrorAction SilentlyContinue }
# this run's own DLL: the installed one (which needs to know EEAIMOD_DLL) loads the copy and stays out of the way
if ($Dll) {
    if (-not [System.IO.Path]::IsPathRooted($Dll)) { $Dll = Join-Path $repo $Dll }
    $why = if (-not (Test-Path $Dll)) { "-Dll not found: $Dll" }
           elseif (-not (Select-String -Path "$g\eeaimod.dll" -Pattern 'EEAIMOD_DLL' -SimpleMatch -Quiet)) {
               "the installed eeaimod.dll predates -Dll (install a current build once: sh dll/build.sh install)" }
    if ($why) { $why; exit 1 }
    $env:EEAIMOD_DLL = "$repo\work\eeaimod_slot$Slot.dll"
    try { Copy-Item $Dll $env:EEAIMOD_DLL -Force -ErrorAction Stop }
    catch { "can't copy -Dll to $($env:EEAIMOD_DLL): $_"; exit 1 }
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
# setup=<file>[;<file>...]: relative to the repository (scenarios/x.eeai), made absolute for the game
for ($i = 0; $i -lt $extra.Count; $i++) {
    if ($extra[$i] -notmatch '^\s*setup\s*=(.*)$') { continue }
    $files = @()
    foreach ($f in $Matches[1].Split(';')) {
        $f = $f.Trim()
        if (-not $f) { continue }
        if ($f -match '^_aimod[\\/]') { "setup=$f`: relative to the repository now ($($f.Substring(7)))"; exit 1 }
        if (-not [System.IO.Path]::IsPathRooted($f)) { $f = Join-Path $repo $f }
        if (-not (Test-Path $f)) { "setup: no file $f"; exit 1 }
        $files += $f
    }
    $extra[$i] = "setup=" + ($files -join ';')
}
if ($Scenario) { $extra += "scenario_mode=1" }
$extra += "config=$Config", "ai_dir=$AiDir"
$keys = $extra | ForEach-Object { ($_ -split '=', 2)[0].Trim() }
$lines = Get-Content "$repo\autotest.ini" | Where-Object { $keys -notcontains (($_ -split '=', 2)[0].Trim()) }
Set-Content "$sdir\aimod_autotest.ini" (@($lines) + $extra)
# the game started from here sees the slot (and a slot-0 run none, whatever this shell has set)
if ($Slot) { $env:EEAIMOD_SLOT = "$Slot"; $env:EEAIMOD_SLOT_DIR = $sdir }
else { Remove-Item Env:EEAIMOD_SLOT, Env:EEAIMOD_SLOT_DIR -ErrorAction SilentlyContinue }
$env:EEAIMOD_REPO = $repo   # the repository (texts corpus: its scripts\text_engine_cases.txt)
$env:EEAIMOD_UNREADY = "1"   # a known build that isn't ready yet plays here (game/api/exe.c) - never in normal play
# headless: the mod's dinput.dll keeps every window of the game from taking the focus or a taskbar button, from the
# process start on (dll/base/headless_win.c; eeaimod.dll alone only from its load on)
if ($extra -contains "headless=1") { $env:EEAIMOD_HEADLESS = "1" } else { Remove-Item Env:EEAIMOD_HEADLESS -ErrorAction SilentlyContinue }
try {
    # an existing log in the game folder is from normal play (tests archive theirs below): keep it, never delete it;
    # in a slot's folder: a crashed run's
    if (Test-Path "$sdir\aimod_econ.csv") {
        $kept = "$repo\work\$(if ($Slot) { 'slot' + $Slot + '_left_' } else { 'play_' })$(Get-Date -Format yyyyMMdd_HHmmss).csv"
        Move-Item "$sdir\aimod_econ.csv" $kept -Force
        "kept the previous log: $kept"
    }
    Wait-Reservation
    $t0 = Get-Date
    # the game gets handles of its own to the slot and to eeaimod_game_<pid> (DuplicateHandle): they live as long as
    # it does, whatever happens to this runner
    Add-Type @"
using System; using System.Runtime.InteropServices; using System.Threading;
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
  [DllImport("kernel32")] static extern IntPtr GetCurrentProcess();
  [DllImport("kernel32")] static extern bool DuplicateHandle(IntPtr sp, IntPtr sh, IntPtr tp, out IntPtr th, int a, bool inh, int o);
  static bool Give(IntPtr hp, IntPtr h) {   // a copy of the handle h in process hp (DUPLICATE_SAME_ACCESS)
    IntPtr th; return DuplicateHandle(GetCurrentProcess(), h, hp, out th, 0, false, 2);
  }
  // the game process hp (its pid) gets a handle to the slot and its eeaimod_game_<pid>, which only it holds then
  public static bool Mark(IntPtr hp, int pid, WaitHandle slot) {
    bool made;
    using (EventWaitHandle m = new EventWaitHandle(false, EventResetMode.ManualReset, "Local\\eeaimod_game_" + pid, out made))
      return Give(hp, slot.SafeWaitHandle.DangerousGetHandle()) & Give(hp, m.SafeWaitHandle.DangerousGetHandle());
  }
  static bool Patch(IntPtr hp, int at, byte[] want, byte[] put) {   // only while the bytes are the expected ones
    byte[] cur = new byte[want.Length]; IntPtr n; int old;
    if (!ReadProcessMemory(hp, new IntPtr(at), cur, cur.Length, out n)) return false;
    for (int i = 0; i < want.Length; i++) if (cur[i] != want[i]) return false;
    return VirtualProtectEx(hp, new IntPtr(at), put.Length, 0x40, out old) && WriteProcessMemory(hp, new IntPtr(at), put, put.Length, out n);
  }
  // splash: the loading splash off (its jne -> jmp); multi: the single-instance check off (its je nopped - the mutex,
  // "StainlessSteelStudiosPresentsEmpireEarth" in the 2017 update, is created before the engine loads our DLL)
  public static bool Start(string exe, string args, string dir, bool splash, int splashAt, bool multi, int multiAt,
                          byte[] multiWant, WaitHandle slot, out bool splashOk, out bool multiOk, out int pid) {
    SI si = new SI(); si.cb = Marshal.SizeOf(si); PI pi;
    if (splash) si.flags = 0x80;   // headless: STARTF_FORCEOFFFEEDBACK - no busy cursor while it starts
    if (!CreateProcessW(exe, "\"" + exe + "\" " + args, IntPtr.Zero, IntPtr.Zero, false, 4, IntPtr.Zero, dir, ref si, out pi))
      throw new Exception("CreateProcess failed: " + Marshal.GetLastWin32Error());
    splashOk = splash && Patch(pi.hp, splashAt, new byte[] { 0x75, 0x2F }, new byte[] { 0xEB });
    multiOk = multi && Patch(pi.hp, multiAt, multiWant, new byte[] { 0x90, 0x90, 0x90, 0x90, 0x90, 0x90 });
    bool given = Mark(pi.hp, pi.pid, slot);   // before it runs: no test game without its handles
    pid = pi.pid;
    ResumeThread(pi.ht); CloseHandle(pi.ht); CloseHandle(pi.hp);
    return given;
  }
}
"@
    # headless or beside other games: start suspended and patch before the game runs (else a plain start)
    if ($extra -contains "headless=1" -or $Slot -or $others) {
        # headless: start suspended and switch off the loading splash before the game runs - it comes up before
        # the engine loads our DLL, centred on screen, and takes the focus (its jne -> jmp, see display.c)
        $headless = $extra -contains "headless=1"; $multi = [bool]($Slot -or $others)
        $splashOk = $false; $multiOk = $false; $gamePid = 0
        $given = [SuspendedStart]::Start("$g\$Exe", "-loadgame `"$Save`"", $g, $headless, $build.Splash, $multi,
                                         $build.Multi, $build.MultiBytes, $slotHeld, [ref]$splashOk, [ref]$multiOk, [ref]$gamePid)
        $p = Get-Process -Id $gamePid
        $null = $p.Handle   # keep a handle, else ExitCode is lost
        if ($headless -and -not $splashOk) { "warning: loading splash not switched off (unexpected bytes at $('{0:X}' -f $build.Splash))" }
        if ($multi -and -not $multiOk) { "warning: single-instance check not switched off (unexpected bytes at $('{0:X}' -f $build.Multi))" }
    } else {
        $psi = New-Object System.Diagnostics.ProcessStartInfo
        $psi.FileName = "$g\$Exe";$psi.Arguments = "-loadgame `"$Save`""
        $psi.WorkingDirectory = $g; $psi.UseShellExecute = $false
        $p = [System.Diagnostics.Process]::Start($psi)
        $given = [SuspendedStart]::Mark($p.Handle, $p.Id, $slotHeld)   # it runs already: a runner's child meanwhile
    }
    if (-not $given) { "warning: the game didn't get its slot handles - a killed runner would free its slot" }
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
    # the slot is this run's until it ends: its test ini can go (the game read it at its start)
    Remove-Item "$sdir\aimod_autotest.ini" -Confirm:$false -ErrorAction SilentlyContinue
    if (-not (Other-Runners)) { Restore-Sound }   # the last slot running restores the player's sound
}
$stamp = Get-Date -Format "yyyyMMdd_HHmmss"
$dest = "$repo\work\autotest_${stamp}$(if ($Slot) { "_s$Slot" })${Tag}.csv"
if (Test-Path "$sdir\aimod_econ.csv") { Move-Item "$sdir\aimod_econ.csv" $dest }
"finished in $secs s real time, exit code $($p.ExitCode), log: $dest"

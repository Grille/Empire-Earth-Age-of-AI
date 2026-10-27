# The own renderer's picture against a wrapper's (docs/renderer.md, step 4), part 1: starts the same game (game_seed)
# in two test slots and returns - A draws through a ddraw.dll loaded from outside the game folder (autotest
# wrapper_dll; default DDrawCompat, local/work/ddraw.dll.ddrawcompat), B through the own renderer.
# tests/probes/render_compare.eeai pauses both at the same simulation steps; part 2 shoots and compares:
#   sh scripts/test/render_compare.sh <stamp> [slot]   (the stamp and slot this prints)
# usage: powershell -File scripts/test/render_compare.ps1 [-Slot 27] [-Keys "a=1,b=2"] [-Dll path] [-Wrapper path]
#        [-Map "random_map=Continental,map_size=0,ai_count=3,seed=101"] [-GameSeed 1] [-Epoch 0]
#        [-OwnOnly]: only B - the same game's wrapper shots taken from an earlier run (part 2: REF=<its dir>)
param([int]$Slot = 27, [string]$Keys = "", [string]$Dll = $env:DLL, [string]$Wrapper = "local\work\ddraw.dll.ddrawcompat",
      [string]$Map = "random_map=Continental,map_size=0,ai_count=3,seed=101", [int]$GameSeed = 1, [int]$Epoch = 0, [switch]$OwnOnly)
$repo = Resolve-Path "$PSScriptRoot\..\.."
Set-Location $repo
$stamp = Get-Date -Format "yyyyMMdd_HHmmss"
New-Item -ItemType Directory -Force "local\work\rc_$stamp", "local\work\rc_wrapper" | Out-Null
Copy-Item $Wrapper "local\work\rc_wrapper\ddraw.dll" -Force
$wpath = Join-Path $repo "local\work\rc_wrapper\ddraw.dll"
$ini = "$Map,teams=ffa,start_epoch=$Epoch,ai_cheat=0,headless=1,game_seed=$GameSeed,quit_minutes=13,setup=tests/probes/render_compare.eeai"
if ($Keys) { $ini = "$ini,$Keys" }
$runs = @(@($Slot, "wrapper", "display_renderer=wrapper,wrapper_dll=$wpath"), @(($Slot + 1), "own", "display_renderer=own"))
if ($OwnOnly) { $runs = @(,$runs[1]) }
foreach ($r in $runs) {
    $pargs = @("-NoProfile", "-File", "`"$repo\scripts\test\run_test.ps1`"", "-Slot", $r[0],
              "-Save", "newgame.ees", "-TimeoutSec", "1500", "-Tag", "_rc$($stamp)_$($r[1])", "-IniExtra", "`"$ini,$($r[2])`"")
    if ($Dll) { $pargs += @("-Dll", "`"$Dll`"") }
    Start-Process -WindowStyle Hidden powershell -ArgumentList $pargs `
        -RedirectStandardOutput "local\work\rc_$stamp\run_$($r[1]).txt" -RedirectStandardError "local\work\rc_$stamp\run_$($r[1])_err.txt"
}
"started: sh scripts/test/render_compare.sh $stamp $Slot"

# Keeps the test output in the repository's work\ from growing without end: run_test.ps1 calls it after archiving a
# log (at most once an hour, -Quiet), by hand: powershell -File scripts/work_prune.ps1 [-WhatIf]. Parallel A/B runs
# wrote 30 GB of logs a day (2026-10-23: 54,000 logs, 77 GB, none ever deleted).
#   the test logs (autotest_*.csv.gz - older ones plain -, slot<n>_left_*.csv): older than -Days (env
#     EEAIMOD_KEEP_DAYS, default 7) deleted, then the oldest until they hold less than -GB together (env
#     EEAIMOD_KEEP_GB, default 10; gzipped, a day of parallel A/Bs is ~4.4 GB) - an A/B is scored right after its
#     runs, a week is history enough; a log to keep for longer goes into work\keep\
#   a slot's dumps (work\slot<n>\aimod_dumps\*, the tests' only - the player's game folder keeps its own): older
#     than -Days
# Never touched: work\keep\, play_*.csv (the player's own games' logs), everything else in work\.
param([double]$Days = $(if ($env:EEAIMOD_KEEP_DAYS) { [double]$env:EEAIMOD_KEEP_DAYS } else { 7 }),
      [double]$GB = $(if ($env:EEAIMOD_KEEP_GB) { [double]$env:EEAIMOD_KEEP_GB } else { 10 }),
      [switch]$Hourly, [switch]$Quiet, [switch]$WhatIf)
$work = Join-Path (Resolve-Path "$PSScriptRoot\..").Path "work"
if (-not (Test-Path $work)) { exit 0 }
$stamp = "$work\prune.stamp"
if ($Hourly -and (Test-Path $stamp) -and ((Get-Date) - (Get-Item $stamp).LastWriteTime).TotalHours -lt 1) { exit 0 }
if (-not $WhatIf) { Set-Content $stamp (Get-Date -Format s) }   # first: runners finishing together prune once
$before = (Get-Date).AddDays(-$Days)
$gone = @()
# a log a running game still writes is in its slot's folder, never here: everything here is finished
$logs = @(Get-ChildItem $work -File -Filter *.csv* | Where-Object { $_.Name -like 'autotest_*' -or $_.Name -like 'slot*_left_*' } |
          Sort-Object LastWriteTime)
$old = @($logs | Where-Object { $_.LastWriteTime -lt $before })
$gone += $old
$left = @($logs | Where-Object { $_.LastWriteTime -ge $before })
$sum = ($left | Measure-Object Length -Sum).Sum
foreach ($f in $left) {   # oldest first
    if ($sum -le $GB * 1GB) { break }
    $gone += $f; $sum -= $f.Length
}
foreach ($d in Get-ChildItem $work -Directory -Filter 'slot*') {
    if (Test-Path "$($d.FullName)\aimod_dumps") {
        $gone += @(Get-ChildItem "$($d.FullName)\aimod_dumps" -File | Where-Object { $_.LastWriteTime -lt $before })
    }
}
$size = ($gone | Measure-Object Length -Sum).Sum
$line = "{0} {1} files, {2:N1} GB older than {3} days or over {4} GB{5}" -f (Get-Date -Format 'yyyy-MM-dd HH:mm'),
        $gone.Count, ($size / 1GB), $Days, $GB, $(if ($WhatIf) { " (would be deleted)" } else { " deleted" })
if (-not $WhatIf) {
    foreach ($f in $gone) { Remove-Item -LiteralPath $f.FullName -Force -Confirm:$false -ErrorAction SilentlyContinue }
    if ($gone) { Add-Content "$work\prune.log" $line }
}
if (-not $Quiet) { $line }

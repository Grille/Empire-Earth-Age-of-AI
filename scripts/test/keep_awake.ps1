# Keep the PC from going to sleep while a Claude Code session works (the screen may still turn off).
# usage: powershell -File scripts/test/keep_awake.ps1 [-Owner <pid>]
# Without -Owner it walks up its own parent processes to the first claude.exe - the session that started it - and
# holds ES_CONTINUOUS | ES_SYSTEM_REQUIRED until that process ends (Windows drops the request when this script exits).
# run_test.ps1 does the same for a single test run; this covers the time between runs (analysis, builds).
param([int]$Owner = 0)
if (-not $Owner) {
    $p = Get-CimInstance Win32_Process -Filter "ProcessId=$PID"
    while ($p -and $p.Name -ne 'claude.exe') { $p = Get-CimInstance Win32_Process -Filter "ProcessId=$($p.ParentProcessId)" }
    if (-not $p) { "no claude.exe among the parent processes - pass -Owner <pid>"; exit 1 }
    $Owner = [int]$p.ProcessId
}
Add-Type -Namespace EEAimod -Name Awake -MemberDefinition '[DllImport("kernel32.dll")] public static extern uint SetThreadExecutionState(uint f);'
[void][EEAimod.Awake]::SetThreadExecutionState([uint32]"0x80000001")
"keeping the PC awake while process $Owner runs"
while (Get-Process -Id $Owner -ErrorAction SilentlyContinue) { Start-Sleep -Seconds 60 }
"process $Owner ended - sleep allowed again"

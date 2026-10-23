# Captures the Empire Earth window content via PrintWindow (works while covered) to a PNG.
param([string]$Out = "$PSScriptRoot\..\..\local\work\screen.png")
Add-Type -AssemblyName System.Drawing
Add-Type @"
using System; using System.Runtime.InteropServices;
public class W32 {
  [StructLayout(LayoutKind.Sequential)] public struct RECT { public int L, T, R, B; }
  [DllImport("user32.dll")] public static extern bool GetClientRect(IntPtr h, out RECT r);
  [DllImport("user32.dll")] public static extern bool PrintWindow(IntPtr h, IntPtr hdc, uint flags);
}
"@
$p = Get-Process -Name "Empire Earth" -ErrorAction SilentlyContinue | Select-Object -First 1
if (-not $p -or $p.MainWindowHandle -eq 0) { "game window not found"; exit 1 }
$h = $p.MainWindowHandle
$r = New-Object W32+RECT
[void][W32]::GetClientRect($h, [ref]$r)
$bmp = New-Object System.Drawing.Bitmap ([Math]::Max(1,$r.R)), ([Math]::Max(1,$r.B))
$g = [System.Drawing.Graphics]::FromImage($bmp)
$hdc = $g.GetHdc()
$ok = [W32]::PrintWindow($h, $hdc, 3)   # PW_CLIENTONLY | PW_RENDERFULLCONTENT
$g.ReleaseHdc($hdc)
$bmp.Save($Out, [System.Drawing.Imaging.ImageFormat]::Png)
"saved $Out ($($r.R) x $($r.B)) printwindow=$ok"

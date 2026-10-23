# Convert a BMP (e.g. from the control pipe's "shot" command) to PNG, optionally scaled to a maximum width.
# usage: powershell -File scripts/test/bmp2png.ps1 <in.bmp> <out.png> [maxWidth]
param([string]$In, [string]$Out, [int]$MaxWidth = 0)
Add-Type -AssemblyName System.Drawing
$b = [Drawing.Bitmap]::FromFile((Resolve-Path $In))
$w = $b.Width; $h = $b.Height
if ($MaxWidth -gt 0 -and $w -gt $MaxWidth) { $h = [int]($h * $MaxWidth / $w); $w = $MaxWidth }
$s = New-Object Drawing.Bitmap $b, $w, $h
$s.Save($Out, [Drawing.Imaging.ImageFormat]::Png)
$s.Dispose(); $b.Dispose()

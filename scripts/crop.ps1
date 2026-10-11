# Crop a region of an image to PNG: crop.ps1 <in> <out> <x> <y> <w> <h> [scale]
param([string]$In, [string]$Out, [int]$X, [int]$Y, [int]$W, [int]$H, [double]$Scale = 1.0)
Add-Type -AssemblyName System.Drawing
$b = [Drawing.Bitmap]::FromFile((Resolve-Path $In))
$c = $b.Clone((New-Object Drawing.Rectangle $X, $Y, $W, $H), $b.PixelFormat)
$s = New-Object Drawing.Bitmap $c, ([int]($W * $Scale)), ([int]($H * $Scale))
$s.Save($Out, [Drawing.Imaging.ImageFormat]::Png)

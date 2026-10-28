param([string]$InputPath = "$PSScriptRoot\..\assets\duck-original.png", [string]$OutputPath = "$PSScriptRoot\..\assets\lost-eye.ico")
$ErrorActionPreference = 'Stop'
Add-Type -AssemblyName System.Drawing
$original = [Drawing.Bitmap]::FromFile((Resolve-Path $InputPath))
$sizes = @(16,24,32,48,64,128,256)
$frames = @()
try {
  foreach ($size in $sizes) {
    $bitmap = New-Object Drawing.Bitmap($size,$size,[Drawing.Imaging.PixelFormat]::Format32bppArgb)
    $graphics = [Drawing.Graphics]::FromImage($bitmap)
    $graphics.CompositingQuality = [Drawing.Drawing2D.CompositingQuality]::HighQuality
    $graphics.InterpolationMode = [Drawing.Drawing2D.InterpolationMode]::HighQualityBicubic
    $graphics.PixelOffsetMode = [Drawing.Drawing2D.PixelOffsetMode]::HighQuality
    $graphics.DrawImage($original,0,0,$size,$size)
    $stream = New-Object IO.MemoryStream
    $bitmap.Save($stream,[Drawing.Imaging.ImageFormat]::Png)
    $frames += ,$stream.ToArray()
    $stream.Dispose(); $graphics.Dispose(); $bitmap.Dispose()
  }
  $file = [IO.File]::Create([IO.Path]::GetFullPath($OutputPath))
  $writer = New-Object IO.BinaryWriter($file)
  $writer.Write([uint16]0); $writer.Write([uint16]1); $writer.Write([uint16]$sizes.Count)
  $offset = 6 + 16 * $sizes.Count
  for ($i = 0; $i -lt $sizes.Count; $i++) {
    $dimension = if ($sizes[$i] -eq 256) { 0 } else { $sizes[$i] }
    $writer.Write([byte]$dimension); $writer.Write([byte]$dimension)
    $writer.Write([byte]0); $writer.Write([byte]0); $writer.Write([uint16]1); $writer.Write([uint16]32)
    $writer.Write([uint32]$frames[$i].Length); $writer.Write([uint32]$offset)
    $offset += $frames[$i].Length
  }
  foreach ($frame in $frames) { $writer.Write([byte[]]$frame) }
  $writer.Dispose(); $file.Dispose()
} finally { $original.Dispose() }
Write-Host "Created multi-resolution icon: $OutputPath"

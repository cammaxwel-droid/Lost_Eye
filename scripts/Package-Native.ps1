param([string]$OutputName='Lost-Eye-0.3.0-win-x64')
$ErrorActionPreference='Stop'
$root=(Resolve-Path "$PSScriptRoot\..").Path
if($OutputName -notmatch '^[a-zA-Z0-9._-]+$'){throw 'Use a simple output name.'}
$out=Join-Path $root "artifacts\$OutputName"
$zip=Join-Path $root "artifacts\$OutputName.zip"
if(!(Test-Path -LiteralPath (Join-Path $out 'Lost Eye.exe'))){throw 'Run Build-Native.ps1 first.'}
if(Test-Path -LiteralPath $zip){throw 'ZIP already exists; choose another output name to preserve it.'}
$source=Join-Path $root ('artifacts\native-source-'+[Guid]::NewGuid().ToString('N'))
New-Item -ItemType Directory -Path $source,(Join-Path $source 'native'),(Join-Path $source 'scripts'),(Join-Path $source 'assets'),(Join-Path $source 'licenses'),(Join-Path $out 'licenses') | Out-Null
Copy-Item -LiteralPath (Join-Path $root 'native\LostEye.cpp'),(Join-Path $root 'native\LostEye.rc'),(Join-Path $root 'native\app.manifest'),(Join-Path $root 'native\THIRD-PARTY-NOTICES.md'),(Join-Path $root 'native\VERIFICATION.md'),(Join-Path $root 'native\Install Lost Eye.cmd') -Destination (Join-Path $source 'native')
Copy-Item -LiteralPath (Join-Path $root 'scripts\Build-Native.ps1'),(Join-Path $root 'scripts\Package-Native.ps1') -Destination (Join-Path $source 'scripts')
Copy-Item -LiteralPath (Join-Path $root 'assets\lost-eye.ico'),(Join-Path $root 'assets\duck-original.png'),(Join-Path $root 'assets\GENERATION.md') -Destination (Join-Path $source 'assets')
Copy-Item -LiteralPath (Join-Path $root 'LICENSE'),(Join-Path $root 'README.md') -Destination $source
Copy-Item -LiteralPath (Join-Path $root 'licenses\GPL-2.0.txt') -Destination (Join-Path $source 'licenses')
Copy-Item -LiteralPath (Join-Path $root 'LICENSE'),(Join-Path $root 'README.md'),(Join-Path $root 'native\THIRD-PARTY-NOTICES.md'),(Join-Path $root 'native\VERIFICATION.md'),(Join-Path $root 'native\Install Lost Eye.cmd') -Destination $out
Copy-Item -LiteralPath (Join-Path $root 'licenses\GPL-2.0.txt') -Destination (Join-Path $out 'licenses')
Compress-Archive -Path (Join-Path $source '*') -DestinationPath (Join-Path $out 'Lost-Eye-native-source.zip') -CompressionLevel Optimal
Compress-Archive -Path (Join-Path $out '*') -DestinationPath $zip -CompressionLevel Optimal
Get-Item -LiteralPath $zip | Select-Object FullName,Length
Get-FileHash -LiteralPath $zip -Algorithm SHA256

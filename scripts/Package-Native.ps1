param([string]$OutputName = 'Lost-Eye-0.3.1-win-x64')
$ErrorActionPreference = 'Stop'
Import-Module (Join-Path $PSHOME 'Modules\Microsoft.PowerShell.Utility\Microsoft.PowerShell.Utility.psd1') -ErrorAction Stop
Import-Module (Join-Path $PSHOME 'Modules\Microsoft.PowerShell.Archive\Microsoft.PowerShell.Archive.psd1') -ErrorAction Stop
$root = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
if ($OutputName -notmatch '^[a-zA-Z0-9][a-zA-Z0-9._-]*$') { throw 'Use a simple output name without directory separators.' }
$out = Join-Path $root "artifacts\$OutputName"
$zip = Join-Path $root "artifacts\$OutputName.zip"
$exe = Join-Path $out 'Lost Eye.exe'
$record = Join-Path $root "artifacts\$OutputName-build\build-info.json"
if (!(Test-Path -LiteralPath $exe) -or !(Test-Path -LiteralPath $record)) { throw 'Run Build-Native.ps1 with this output name first.' }
if (Test-Path -LiteralPath $zip) { throw 'ZIP already exists; build with a new output name to preserve it.' }
if (Test-Path -LiteralPath ($zip + '.sha256')) { throw 'ZIP checksum already exists; build with a new output name to preserve it.' }

# Refuse to label changed sources as the matching source for an earlier executable.
$manifest = Get-Content -LiteralPath $record -Raw | ConvertFrom-Json
if ((Get-FileHash -LiteralPath $exe -Algorithm SHA256).Hash -ne $manifest.ExecutableSHA256) { throw 'The executable changed after building. Rebuild with a new output name.' }
foreach ($inputFile in $manifest.Inputs) {
    if ((Get-FileHash -LiteralPath (Join-Path $root $inputFile.Path) -Algorithm SHA256).Hash -ne $inputFile.SHA256) {
        throw "Build input changed: $($inputFile.Path). Rebuild with a new output name before packaging."
    }
}

# Fresh staging keeps old packages and any old files in build folders untouched.
$stage = Join-Path $root ('artifacts\native-package-' + [Guid]::NewGuid().ToString('N'))
$source = Join-Path $stage 'source'
$portable = Join-Path $stage 'portable'
New-Item -ItemType Directory -Path $source,$portable,(Join-Path $source 'native'),(Join-Path $source 'scripts'),(Join-Path $source 'assets') | Out-Null
$documents = @('LICENSE','README.md','BUILDING.md','LICENSING.md','SOURCES.md','THIRD-PARTY-NOTICES.md','VERIFICATION.md')
foreach ($document in $documents) {
    Copy-Item -LiteralPath (Join-Path $root $document) -Destination $source
    Copy-Item -LiteralPath (Join-Path $root $document) -Destination $portable
}
Copy-Item -LiteralPath (Join-Path $root 'Build Lost Eye.cmd') -Destination $source
foreach ($file in @('LostEye.cpp','LostEye.rc','app.manifest','THIRD-PARTY-NOTICES.md','VERIFICATION.md','Install Lost Eye.cmd')) {
    Copy-Item -LiteralPath (Join-Path $root "native\$file") -Destination (Join-Path $source 'native')
}
foreach ($file in @('Build-Native.ps1','Package-Native.ps1')) {
    Copy-Item -LiteralPath (Join-Path $root "scripts\$file") -Destination (Join-Path $source 'scripts')
}
foreach ($file in @('lost-eye.ico','duck-original.png','GENERATION.md')) {
    Copy-Item -LiteralPath (Join-Path $root "assets\$file") -Destination (Join-Path $source 'assets')
}
Copy-Item -LiteralPath $exe,(Join-Path $root 'native\Install Lost Eye.cmd') -Destination $portable
Copy-Item -LiteralPath $record -Destination (Join-Path $portable 'BUILD-INFO.json')
Compress-Archive -Path (Join-Path $source '*') -DestinationPath (Join-Path $portable 'Lost-Eye-native-source.zip') -CompressionLevel Optimal
Get-ChildItem -LiteralPath $portable -File -Recurse | Sort-Object FullName | ForEach-Object {
    (Get-FileHash -LiteralPath $_.FullName -Algorithm SHA256).Hash.ToLowerInvariant() + '  ' + $_.FullName.Substring($portable.Length + 1).Replace('\','/')
} | Set-Content -LiteralPath (Join-Path $portable 'SHA256SUMS.txt') -Encoding ASCII
Compress-Archive -Path (Join-Path $portable '*') -DestinationPath $zip -CompressionLevel Optimal
$zipHash = Get-FileHash -LiteralPath $zip -Algorithm SHA256
($zipHash.Hash.ToLowerInvariant() + '  ' + [IO.Path]::GetFileName($zip)) | Set-Content -LiteralPath ($zip + '.sha256') -Encoding ASCII
Get-Item -LiteralPath $zip | Select-Object FullName,Length
$zipHash

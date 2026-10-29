param([string]$OutputName = 'Lost-Eye-0.1.0-win-x64-r2')
$ErrorActionPreference = 'Stop'
$root = (Resolve-Path "$PSScriptRoot\..").Path
if ($OutputName -notmatch '^[a-zA-Z0-9._-]+$') { throw 'Use a simple output name, not a path.' }
$stage = Join-Path $root "artifacts\$OutputName"
$zip = "$stage.zip"
if ((Test-Path -LiteralPath $stage) -or (Test-Path -LiteralPath $zip)) { throw 'Output already exists. Preserve it and choose a new -OutputName.' }
$upstream = Join-Path $root 'artifacts\upstream'
New-Item -ItemType Directory -Path $upstream -Force | Out-Null
$downloads = @(
  @{Name='vlc-3.0.24.tar.xz'; Url='https://download.videolan.org/pub/videolan/vlc/3.0.24/vlc-3.0.24.tar.xz'; Hash='E7CAB503D1D7D5849B89D2CF0E1EE60D0EF6D012407791B644B9CFC0CC225FDF'},
  @{Name='libvlcsharp-3.10.2-source.zip'; Url='https://codeload.github.com/videolan/libvlcsharp/zip/3082c09ca955131c705fc4767fde03e9c4a14999'; Hash='7C7B56D71721E7F3F12607E629AECA9F8416119A24CD4D016CF7F8F1F9940336'}
)
foreach ($item in $downloads) {
  $destination = Join-Path $upstream $item.Name
  if (!(Test-Path -LiteralPath $destination)) { Invoke-WebRequest -UseBasicParsing $item.Url -OutFile $destination }
  if ((Get-FileHash -LiteralPath $destination -Algorithm SHA256).Hash -ne $item.Hash) { throw "Source checksum failed: $($item.Name)" }
}
Push-Location $root
try {
  dotnet restore src\LostEye\LostEye.csproj --locked-mode --runtime win-x64
  if ($LASTEXITCODE -ne 0) { throw 'Restore failed' }
  dotnet publish src\LostEye\LostEye.csproj -c Release -r win-x64 --self-contained true --no-restore -o $stage
  if ($LASTEXITCODE -ne 0) { throw 'Publish failed' }
  dotnet run --project tools\PluginCache\PluginCache.csproj -c Release -- (Join-Path $stage 'libvlc\win-x64')
  if ($LASTEXITCODE -ne 0) { throw 'Native plugin cache generation failed' }
  # This script builds the retained GPL .NET/LibVLC release, not native Lost Eye.
  Copy-Item legacy\README.md,legacy\THIRD-PARTY-NOTICES.md,legacy\SOURCES.md,legacy\VERIFICATION.md -Destination $stage
  Copy-Item licenses\LEGACY-GPL-NOTICE.txt -Destination (Join-Path $stage 'LICENSE')
  Copy-Item licenses -Destination $stage -Recurse
  Copy-Item scripts\'Register Open With.cmd',scripts\'Remove Open With.cmd' -Destination $stage
  $sources = Join-Path $stage 'sources'
  New-Item -ItemType Directory -Path $sources | Out-Null
  foreach ($item in $downloads) { Copy-Item (Join-Path $upstream $item.Name) -Destination $sources }
  $snapshot = Join-Path $root "artifacts\$OutputName-source"
  if (Test-Path -LiteralPath $snapshot) { throw 'Source snapshot already exists.' }
  New-Item -ItemType Directory -Path $snapshot | Out-Null
  foreach ($folder in @('src','assets','scripts','tests','tools','licenses','legacy')) {
    Get-ChildItem -LiteralPath (Join-Path $root $folder) -File -Recurse | Where-Object {
      $_.FullName -notmatch '[\\/](bin|obj)[\\/]' -and $_.Name -notin @('Build-Native.ps1','Package-Native.ps1')
    } | ForEach-Object {
      $relative = $_.FullName.Substring($root.Length + 1)
      $destination = Join-Path $snapshot $relative
      New-Item -ItemType Directory -Path (Split-Path $destination) -Force | Out-Null
      Copy-Item -LiteralPath $_.FullName -Destination $destination
    }
  }
  Copy-Item legacy\README.md,legacy\THIRD-PARTY-NOTICES.md,legacy\SOURCES.md,legacy\VERIFICATION.md,.gitignore -Destination $snapshot
  Copy-Item licenses\LEGACY-GPL-NOTICE.txt -Destination (Join-Path $snapshot 'LICENSE')
  Add-Type -AssemblyName System.IO.Compression.FileSystem
  [IO.Compression.ZipFile]::CreateFromDirectory($snapshot, (Join-Path $sources 'Lost-Eye-source.zip'), [IO.Compression.CompressionLevel]::Optimal, $false)
  $files = Get-ChildItem -LiteralPath $stage -File -Recurse | Sort-Object FullName
  $manifest = foreach ($file in $files) { '{0}  {1}' -f (Get-FileHash -LiteralPath $file.FullName -Algorithm SHA256).Hash, $file.FullName.Substring($stage.Length + 1) }
  $manifest | Set-Content -LiteralPath (Join-Path $stage 'SHA256SUMS.txt') -Encoding UTF8
  [IO.Compression.ZipFile]::CreateFromDirectory($stage, $zip, [IO.Compression.CompressionLevel]::Optimal, $false)
  '{0}  {1}' -f (Get-FileHash -LiteralPath $zip -Algorithm SHA256).Hash, (Split-Path $zip -Leaf) | Set-Content "$zip.sha256" -Encoding ASCII
  Get-Item -LiteralPath $zip | Select-Object FullName,Length
} finally { Pop-Location }


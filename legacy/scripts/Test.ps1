$ErrorActionPreference = 'Stop'
$root = (Resolve-Path "$PSScriptRoot\..").Path
Push-Location $root
try {
  New-Item -ItemType Directory -Path artifacts\fixtures -Force | Out-Null
  ffmpeg -hide_banner -loglevel error -f lavfi -i 'testsrc2=size=640x360:rate=30' -f lavfi -i 'sine=frequency=440:sample_rate=48000' -t 8 -c:v libx264 -pix_fmt yuv420p -c:a aac -y 'artifacts\fixtures\sample video.mp4'
  if ($LASTEXITCODE -ne 0) { throw 'MP4 fixture generation failed' }
  ffmpeg -hide_banner -loglevel error -i 'artifacts\fixtures\sample video.mp4' -c copy -y artifacts\fixtures\sample.mkv
  if ($LASTEXITCODE -ne 0) { throw 'MKV fixture generation failed' }
  ffmpeg -hide_banner -loglevel error -i 'artifacts\fixtures\sample video.mp4' -c:v libvpx-vp9 -deadline realtime -cpu-used 8 -c:a libopus -y artifacts\fixtures\sample.webm
  if ($LASTEXITCODE -ne 0) { throw 'WebM fixture generation failed' }
  [IO.File]::WriteAllBytes((Join-Path $root 'artifacts\fixtures\empty.mp4'),[byte[]]@())
  'Not a video; deliberately damaged fixture.' | Set-Content artifacts\fixtures\damaged.mp4
  dotnet build tests\LostEye.SmokeTests\LostEye.SmokeTests.csproj -c Release
  if ($LASTEXITCODE -ne 0) { throw 'Test build failed' }
  dotnet run --project tools\PluginCache\PluginCache.csproj -c Release -- (Join-Path $root 'tests\LostEye.SmokeTests\bin\Release\net10.0-windows\win-x64\libvlc\win-x64')
  if ($LASTEXITCODE -ne 0) { throw 'Plugin cache failed' }
  & '.\tests\LostEye.SmokeTests\bin\Release\net10.0-windows\win-x64\LostEye.SmokeTests.exe' '.\artifacts\fixtures' '.\artifacts\verification'
  if ($LASTEXITCODE -ne 0) { throw 'Integration test failed; see artifacts\verification\results.json' }
} finally { Pop-Location }


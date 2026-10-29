param([string]$OutputName='Lost-Eye-0.3.0-win-x64',[switch]$TestsOnly)
$ErrorActionPreference='Stop'
$root=(Resolve-Path "$PSScriptRoot\..").Path
$vswhere='C:\Program Files (x86)\Microsoft Visual Studio\Installer\vswhere.exe'
$vs=& $vswhere -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
if (!$vs) {throw 'Building source requires the Microsoft C++ build tools and Windows SDK. Running the app does not.'}
$vc=Join-Path $vs 'Common7\Tools\VsDevCmd.bat'
$environmentLines=& cmd.exe /d /c "`"$vc`" -no_logo -arch=x64 -host_arch=x64 && set"
foreach($line in $environmentLines){if($line -match '^([^=]+)=(.*)$'){[Environment]::SetEnvironmentVariable($Matches[1],$Matches[2],'Process')}}
if($OutputName -notmatch '^[a-zA-Z0-9._-]+$'){throw 'Use a simple output name.'}
$out=Join-Path $root "artifacts\$OutputName"
$test=Join-Path $root "artifacts\$OutputName-tests"
New-Item -ItemType Directory -Path $out,$test -Force | Out-Null
Push-Location (Join-Path $root 'native')
try {
 & rc.exe /nologo /fo (Join-Path $test 'LostEye.res') LostEye.rc
 if($LASTEXITCODE -ne 0){throw 'Resource compilation failed'}
 $libs=@('user32.lib','gdi32.lib','comctl32.lib','ole32.lib','oleaut32.lib','shell32.lib','shlwapi.lib','advapi32.lib','mfplat.lib','mfuuid.lib','d3d11.lib','dxgi.lib','propsys.lib')
 $flags=@('/nologo','/std:c++20','/EHsc','/O2','/MT','/W4','/utf-8','/D_WIN32_WINNT=0x0A00','/DWINVER=0x0A00','LostEye.cpp',(Join-Path $test 'LostEye.res'))
 if(!$TestsOnly){
 & cl.exe @flags "/Fo$(Join-Path $test 'LostEye.obj')" "/Fe$(Join-Path $out 'Lost Eye.exe')" /link /SUBSYSTEM:WINDOWS /MANIFEST:EMBED /MANIFESTINPUT:app.manifest @libs
 if($LASTEXITCODE -ne 0){throw 'Native app compilation failed'}
 }
 & cl.exe @flags /DLOSTEYE_TEST "/Fo$(Join-Path $test 'Tests.obj')" "/Fe$(Join-Path $test 'LostEye.Tests.exe')" /link /SUBSYSTEM:WINDOWS /MANIFEST:EMBED /MANIFESTINPUT:app.manifest @libs
 if($LASTEXITCODE -ne 0){throw 'Native test compilation failed'}
 & dumpbin.exe /dependents (Join-Path $out 'Lost Eye.exe') | Set-Content (Join-Path $test 'dependencies.txt')
 Get-Item -LiteralPath (Join-Path $out 'Lost Eye.exe') | Select-Object FullName,Length
} finally {Pop-Location}

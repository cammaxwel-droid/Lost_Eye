param(
    [string]$OutputName = 'Lost-Eye-0.3.1-win-x64',
    [switch]$WithTests,
    [switch]$TestsOnly
)
$ErrorActionPreference = 'Stop'
# A launcher can inherit another PowerShell version's module search paths.
# Load this host's built-in hash/JSON commands instead of relying on autoload order.
Import-Module (Join-Path $PSHOME 'Modules\Microsoft.PowerShell.Utility\Microsoft.PowerShell.Utility.psd1') -ErrorAction Stop
$root = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
if ($WithTests -and $TestsOnly) { throw 'Choose -WithTests or -TestsOnly, not both.' }
if ($OutputName -notmatch '^[a-zA-Z0-9][a-zA-Z0-9._-]*$') { throw 'Use a simple output name without directory separators.' }

# A double-click rebuild gets its own folder. Explicit names never overwrite a build.
$artifacts = Join-Path $root 'artifacts'
if (!$PSBoundParameters.ContainsKey('OutputName')) {
    if ((Test-Path -LiteralPath (Join-Path $artifacts $OutputName)) -or
        (Test-Path -LiteralPath (Join-Path $artifacts ($OutputName + '-build'))) -or
        (Test-Path -LiteralPath (Join-Path $artifacts ($OutputName + '-tests')))) {
        $OutputName += '-' + (Get-Date -Format 'yyyyMMdd-HHmmss') + '-' + [Guid]::NewGuid().ToString('N').Substring(0,6)
    }
}
$out = Join-Path $artifacts $OutputName
$build = Join-Path $artifacts ($OutputName + '-build')
$test = Join-Path $artifacts ($OutputName + '-tests')
foreach ($target in @($build, $test)) {
    if (Test-Path -LiteralPath $target) { throw "Output already exists: $target. Choose a new -OutputName to preserve it." }
}
if (!$TestsOnly -and (Test-Path -LiteralPath $out)) { throw "Output already exists: $out. Choose a new -OutputName to preserve it." }

$installerRoot = [Environment]::GetFolderPath('ProgramFilesX86')
$vswhere = Join-Path $installerRoot 'Microsoft Visual Studio\Installer\vswhere.exe'
if (!(Test-Path -LiteralPath $vswhere)) { throw 'Install an appropriately licensed Visual Studio C++ toolchain and Windows SDK first. See BUILDING.md.' }
$vs = & $vswhere -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
if ($LASTEXITCODE -ne 0 -or !$vs) { throw 'No installed MSVC x64/x86 C++ toolset was found. See BUILDING.md.' }
$vc = Join-Path ($vs | Select-Object -First 1) 'Common7\Tools\VsDevCmd.bat'
if (!(Test-Path -LiteralPath $vc)) { throw 'Visual Studio developer environment is missing. Repair the C++ Build Tools installation.' }
$environmentLines = & $env:ComSpec /d /c "call `"$vc`" -no_logo -arch=x64 -host_arch=x64 && set"
if ($LASTEXITCODE -ne 0) { throw 'The Microsoft C++ developer environment could not initialize.' }
foreach ($line in $environmentLines) {
    if ($line -match '^([^=]+)=(.*)$') { [Environment]::SetEnvironmentVariable($Matches[1], $Matches[2], 'Process') }
}
foreach ($tool in @('cl.exe', 'rc.exe', 'dumpbin.exe')) {
    if (!(Get-Command $tool -CommandType Application -ErrorAction SilentlyContinue)) { throw "Missing $tool. Install the C++ toolset and Windows SDK; see BUILDING.md." }
}
if (!$env:WindowsSdkDir -or !$env:WindowsSDKVersion) { throw 'A Windows SDK was not found. Add one using Visual Studio Installer.' }

New-Item -ItemType Directory -Path $build | Out-Null
if (!$TestsOnly) { New-Item -ItemType Directory -Path $out | Out-Null }
if ($WithTests -or $TestsOnly) { New-Item -ItemType Directory -Path $test | Out-Null }
$inputs = @('native\LostEye.cpp', 'native\LostEye.rc', 'native\app.manifest', 'assets\lost-eye.ico', 'scripts\Build-Native.ps1')
$inputHashes = @($inputs | ForEach-Object { [pscustomobject]@{ Path = $_; SHA256 = (Get-FileHash -LiteralPath (Join-Path $root $_) -Algorithm SHA256).Hash } })
Push-Location (Join-Path $root 'native')
try {
    $resource = Join-Path $build 'LostEye.res'
    & rc.exe /nologo /fo $resource LostEye.rc
    if ($LASTEXITCODE -ne 0) { throw 'Resource compilation failed.' }
    $libs = @('user32.lib','gdi32.lib','comctl32.lib','ole32.lib','oleaut32.lib','shell32.lib','shlwapi.lib','advapi32.lib','mfplat.lib','mfuuid.lib','d3d11.lib','dxgi.lib','propsys.lib')
    $flags = @('/nologo','/std:c++20','/EHsc','/O2','/MT','/W4','/utf-8','/D_WIN32_WINNT=0x0A00','/DWINVER=0x0A00','LostEye.cpp',$resource)
    if (!$TestsOnly) {
        $exe = Join-Path $out 'Lost Eye.exe'
        & cl.exe @flags "/Fo$(Join-Path $build 'LostEye.obj')" "/Fe$exe" /link /SUBSYSTEM:WINDOWS /MANIFEST:EMBED /MANIFESTINPUT:app.manifest @libs
        if ($LASTEXITCODE -ne 0) { throw 'Native app compilation failed.' }
        $dependencies = & dumpbin.exe /dependents $exe
        if ($LASTEXITCODE -ne 0) { throw 'Dependency inspection failed.' }
        $dependencies | Set-Content -LiteralPath (Join-Path $build 'dependencies.txt') -Encoding UTF8
        Copy-Item -LiteralPath (Join-Path $root 'native\Install Lost Eye.cmd') -Destination $out
        $compilerVersion = (Get-Item -LiteralPath (Get-Command cl.exe -CommandType Application).Source).VersionInfo.FileVersion
        [pscustomobject]@{
            Target = 'windows-x64'
            Compiler = 'Microsoft C/C++'
            CompilerVersion = $compilerVersion
            VCToolsVersion = $env:VCToolsVersion
            WindowsSDKVersion = $env:WindowsSDKVersion.TrimEnd('\')
            Inputs = $inputHashes
            ExecutableSHA256 = (Get-FileHash -LiteralPath $exe -Algorithm SHA256).Hash
        } | ConvertTo-Json -Depth 4 | Set-Content -LiteralPath (Join-Path $build 'build-info.json') -Encoding UTF8
        Write-Host "`nBuilt: $exe"
        Write-Host "Optional setup: $(Join-Path $out 'Install Lost Eye.cmd')"
        Write-Host 'Setup adds Start and Open with entries; your default player stays unchanged.'
    }
    if ($WithTests -or $TestsOnly) {
        $testExe = Join-Path $test 'LostEye.Tests.exe'
        & cl.exe @flags /DLOSTEYE_TEST "/Fo$(Join-Path $build 'Tests.obj')" "/Fe$testExe" /link /SUBSYSTEM:WINDOWS /MANIFEST:EMBED /MANIFESTINPUT:app.manifest @libs
        if ($LASTEXITCODE -ne 0) { throw 'Native test compilation failed.' }
        Write-Host "Test executable: $testExe"
    }
    Write-Host "Build name: $OutputName"
} finally { Pop-Location }

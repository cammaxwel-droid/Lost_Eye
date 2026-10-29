param([string]$PlayerExecutable)
$ErrorActionPreference = 'Stop'
Import-Module (Join-Path $PSHOME 'Modules\Microsoft.PowerShell.Utility\Microsoft.PowerShell.Utility.psd1') -ErrorAction Stop
$repository = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
if ($PlayerExecutable) { $PlayerExecutable = (Resolve-Path -LiteralPath $PlayerExecutable).Path }
. (Join-Path $repository 'native\Uninstall-LostEye.ps1')
$testId = [Guid]::NewGuid().ToString('N')
$testRoot = Join-Path ([IO.Path]::GetTempPath()) ('LostEye.Uninstall.Tests.!' + $testId)
$registryPath = 'Software\LostEye.Uninstall.Tests.' + $testId
$sandbox = [Microsoft.Win32.Registry]::CurrentUser.CreateSubKey($registryPath)
[void][IO.Directory]::CreateDirectory($testRoot)
$script:checks = 0
$script:caseNumber = 0

function Assert-Test([bool]$Condition, [string]$Message) {
    if (!$Condition) { throw "FAIL: $Message" }
    $script:checks++
    Write-Output "PASS: $Message"
}
function Set-TestRegistry($Root, [string]$Key, [string]$Name, $Value, $Kind = [Microsoft.Win32.RegistryValueKind]::String) {
    $handle = $Root.CreateSubKey($Key)
    try { $handle.SetValue($Name, $Value, $Kind) } finally { $handle.Dispose() }
}
function Write-TestShortcut([string]$Path, [string]$Target) {
    [void][IO.Directory]::CreateDirectory([IO.Path]::GetDirectoryName($Path))
    $shell = New-Object -ComObject WScript.Shell
    try { $link = $shell.CreateShortcut($Path); $link.TargetPath = $Target; $link.Save() }
    finally { if ($link) { [void][Runtime.InteropServices.Marshal]::FinalReleaseComObject($link) }; [void][Runtime.InteropServices.Marshal]::FinalReleaseComObject($shell) }
}
function New-TestFixture {
    $script:caseNumber++
    $caseRoot = Join-Path $testRoot ('case-' + $script:caseNumber)
    $local = Join-Path $caseRoot 'Local'
    $programs = Join-Path $caseRoot 'Start'
    $directory = Join-Path $local 'Programs\Lost Eye\0.3.2'
    [void][IO.Directory]::CreateDirectory($directory)
    [void][IO.Directory]::CreateDirectory($programs)
    $exe = Join-Path $directory 'Lost Eye.exe'
    [IO.File]::Copy($PlayerExecutable, $exe)
    foreach ($name in @('Uninstaller.cmd','Uninstall-LostEye.ps1')) { [IO.File]::Copy((Join-Path $repository "native\$name"), (Join-Path $directory $name)) }
    $root = $sandbox.CreateSubKey('case-' + $script:caseNumber)
    $context = [pscustomobject]@{ LocalAppData = $local; ProgramsDirectory = $programs; RegistryRoot = $root; SourceDirectory = (Join-Path $repository 'native'); ProcessProbe = { @() } }
    $command = '"' + $exe + '" "%1"'
    $icon = '"' + $exe + '",0'
    $app = 'Software\Classes\Applications\Lost Eye.exe'
    $prog = 'Software\Classes\LostEye.Video'
    $caps = 'Software\Lost Eye\Capabilities'
    Set-TestRegistry $root $app 'FriendlyAppName' 'Lost Eye'
    Set-TestRegistry $root $app 'MultiSelectModel' 'Single'
    Set-TestRegistry $root "$app\shell\open\command" '' $command
    Set-TestRegistry $root "$app\DefaultIcon" '' $icon
    Set-TestRegistry $root $prog '' 'Lost Eye video'
    Set-TestRegistry $root "$prog\shell\open\command" '' $command
    Set-TestRegistry $root "$prog\DefaultIcon" '' $icon
    Set-TestRegistry $root $caps 'ApplicationName' 'Lost Eye'
    Set-TestRegistry $root $caps 'ApplicationDescription' 'A simple video player'
    Set-TestRegistry $root $caps 'ApplicationIcon' $icon
    foreach ($ext in @('.mp4','.m4v','.mov','.mkv','.webm','.avi','.wmv','.mpg','.mpeg','.ts','.m2ts','.3gp')) {
        Set-TestRegistry $root "$app\SupportedTypes" $ext ''
        Set-TestRegistry $root "$caps\FileAssociations" $ext 'LostEye.Video'
        Set-TestRegistry $root "Software\Classes\$ext\OpenWithProgids" 'LostEye.Video' ([byte[]]@()) ([Microsoft.Win32.RegistryValueKind]::None)
    }
    Set-TestRegistry $root 'Software\RegisteredApplications' 'Lost Eye' $caps
    Set-TestRegistry $root 'Software\Microsoft\Windows\CurrentVersion\App Paths\Lost Eye.exe' '' $exe
    Set-TestRegistry $root 'Software\Classes\.mp4' '' 'Another.Player'
    Set-TestRegistry $root 'Software\Classes\.mp4\OpenWithProgids' 'Another.Player' ([byte[]]@()) ([Microsoft.Win32.RegistryValueKind]::None)
    Set-TestRegistry $root 'Software\Microsoft\Windows\CurrentVersion\Explorer\FileExts\.mp4\UserChoice' 'ProgId' 'Another.Player'
    Set-TestRegistry $root 'Software\Microsoft\Windows\CurrentVersion\Explorer\FileExts\.mp4\UserChoice' 'Hash' 'preserve-this'
    Write-TestShortcut (Join-Path $programs 'Lost Eye.lnk') $exe
    return [pscustomobject]@{ Context = $context; Root = $root; Directory = $directory; Exe = $exe; Shortcut = (Join-Path $programs 'Lost Eye.lnk'); Command = $command; App = $app; Prog = $prog; Caps = $caps }
}
function Assert-TestRefusal($Fixture, [string]$Name) {
    $failed = $false
    try { [void](Invoke-LostEyeUninstall $Fixture.Context) } catch { $failed = $true }
    Assert-Test $failed "$Name refuses removal"
    Assert-Test ([IO.File]::Exists($Fixture.Exe)) "$Name preserves player"
    Assert-Test (Test-LostEyeRegistryValue $Fixture.Root ($Fixture.App + '\shell\open\command') '' $Fixture.Command String) "$Name preserves registration"
}

try {
    if (!$PlayerExecutable) {
        # A tiny inert versioned fixture avoids opening the real player or installing it.
        $PlayerExecutable = Join-Path $testRoot 'Lost Eye.exe'
        Add-Type -TypeDefinition @'
using System.Reflection;
[assembly: AssemblyTitle("Lost Eye")]
[assembly: AssemblyProduct("Lost Eye")]
[assembly: AssemblyFileVersion("0.3.2.0")]
public class LostEyeUninstallFixture { public static void Main() {} }
'@ -OutputAssembly $PlayerExecutable -OutputType WindowsApplication
    }
    $identity = [Diagnostics.FileVersionInfo]::GetVersionInfo($PlayerExecutable)
    Assert-Test ($identity.ProductName -eq 'Lost Eye' -and $identity.OriginalFilename -eq 'Lost Eye.exe' -and $identity.FileBuildPart -eq 2) 'fixture has expected 0.3.2 identity'

    $f = New-TestFixture
    [void](Invoke-LostEyeUninstall $f.Context)
    Assert-Test (![IO.Directory]::Exists($f.Directory)) 'owned installation folder removed without recursion'
    Assert-Test (![IO.File]::Exists($f.Shortcut)) 'owned Start shortcut removed'
    Assert-Test (!(Test-LostEyeRegistryValue $f.Root ($f.App + '\shell\open\command') '' $f.Command String)) 'owned command removed'
    $emptyApp = $f.Root.OpenSubKey($f.App)
    Assert-Test ($null -eq $emptyApp) 'empty application registration key pruned'
    if ($emptyApp) { $emptyApp.Dispose() }
    Assert-Test (!(Test-LostEyeRegistryValue $f.Root 'Software\Classes\.mp4\OpenWithProgids' 'LostEye.Video' ([byte[]]@()) None)) 'owned OpenWithProgids value removed'
    Assert-Test (Test-LostEyeRegistryValue $f.Root 'Software\Classes\.mp4' '' 'Another.Player' String) 'extension default preserved'
    Assert-Test (Test-LostEyeRegistryValue $f.Root 'Software\Classes\.mp4\OpenWithProgids' 'Another.Player' ([byte[]]@()) None) 'other player preserved'
    Assert-Test (Test-LostEyeRegistryValue $f.Root 'Software\Microsoft\Windows\CurrentVersion\Explorer\FileExts\.mp4\UserChoice' 'Hash' 'preserve-this' String) 'UserChoice preserved'
    Assert-Test ((Invoke-LostEyeUninstall $f.Context).Removed) 'second uninstall is harmless'

    $f = New-TestFixture
    [IO.File]::WriteAllText((Join-Path $f.Directory 'my-video.mp4'), 'preserve')
    Set-TestRegistry $f.Root $f.App 'Unrelated' 'preserve'
    $result = Invoke-LostEyeUninstall $f.Context
    Assert-Test ([IO.File]::ReadAllText((Join-Path $f.Directory 'my-video.mp4')) -eq 'preserve') 'unknown file preserved'
    Assert-Test (Test-LostEyeRegistryValue $f.Root $f.App 'Unrelated' 'preserve' String) 'unknown registry value preserved'
    Assert-Test ($result.RemainingFiles.Count -eq 1) 'remaining file reported'

    $f = New-TestFixture
    $newer = Join-Path ([IO.Path]::GetDirectoryName($f.Directory)) '0.3.3\Lost Eye.exe'
    [void][IO.Directory]::CreateDirectory([IO.Path]::GetDirectoryName($newer))
    [IO.File]::WriteAllText($newer, 'newer player preserved')
    $newCommand = '"' + $newer + '" "%1"'
    Set-TestRegistry $f.Root ($f.App + '\shell\open\command') '' $newCommand
    Set-TestRegistry $f.Root ($f.Prog + '\shell\open\command') '' $newCommand
    Set-TestRegistry $f.Root $f.Caps 'ApplicationIcon' ('"' + $newer + '",0')
    Write-TestShortcut $f.Shortcut $newer
    Write-TestShortcut (Join-Path $f.Directory 'Previous Lost Eye shortcut.lnk') $PlayerExecutable
    [void](Invoke-LostEyeUninstall $f.Context)
    Assert-Test (Test-LostEyeRegistryValue $f.Root ($f.App + '\shell\open\command') '' $newCommand String) 'newer command preserved'
    Assert-Test (Test-LostEyeRegistryValue $f.Root 'Software\Classes\.mp4\OpenWithProgids' 'LostEye.Video' ([byte[]]@()) None) 'newer ProgID registration preserved'
    Assert-Test ((Get-LostEyeShortcutTarget $f.Shortcut) -eq $newer) 'newer shortcut preserved'
    Assert-Test ([IO.File]::Exists((Join-Path $f.Directory 'Previous Lost Eye shortcut.lnk'))) 'backup preserved when shortcut changed'
    Assert-Test ([IO.File]::ReadAllText($newer) -eq 'newer player preserved') 'newer executable preserved'

    $f = New-TestFixture
    Write-TestShortcut (Join-Path $f.Directory 'Previous Lost Eye shortcut.lnk') $PlayerExecutable
    [void](Invoke-LostEyeUninstall $f.Context)
    Assert-Test ((Get-LostEyeShortcutTarget $f.Shortcut) -eq $PlayerExecutable) 'previous shortcut restored'
    Assert-Test ([IO.File]::Exists((Join-Path $f.Directory 'Previous Lost Eye shortcut.lnk'))) 'previous backup preserved after restoration'

    $f = New-TestFixture
    [IO.File]::WriteAllText($f.Exe, 'not a Lost Eye executable')
    Assert-TestRefusal $f 'unowned executable'
    $f = New-TestFixture
    [IO.File]::AppendAllText((Join-Path $f.Directory 'Uninstaller.cmd'), 'changed')
    Assert-TestRefusal $f 'portable-versus-installed script mismatch'
    $f = New-TestFixture
    $running = $f.Exe
    $f.Context.ProcessProbe = { $running }.GetNewClosure()
    Assert-TestRefusal $f 'running installed player'

    $f = New-TestFixture
    [IO.File]::Delete($f.Exe)
    Assert-Test ((Invoke-LostEyeUninstall $f.Context).Removed) 'missing player allows owned registration cleanup'
    Assert-Test (![IO.Directory]::Exists($f.Directory)) 'missing-player installation cleaned'

    $f = New-TestFixture
    $lock = [IO.File]::Open((Join-Path $f.Directory 'Uninstaller.cmd'), [IO.FileMode]::Open, [IO.FileAccess]::Read, [IO.FileShare]::Read)
    try { Assert-TestRefusal $f 'locked uninstaller rollback' } finally { $lock.Dispose() }
    Assert-Test ((Get-LostEyeShortcutTarget $f.Shortcut) -eq $f.Exe) 'rollback restores shortcut'
    Assert-Test ((Invoke-LostEyeUninstall $f.Context).Removed) 'retry succeeds after lock released'

    $f = New-TestFixture
    $junction = Join-Path ([IO.Path]::GetDirectoryName($f.Directory)) 'junction-test'
    [void](New-Item -ItemType Junction -Path $junction -Target $f.Directory)
    $blocked = $false
    try { Assert-LostEyePlainPath (Join-Path $junction 'Lost Eye.exe') } catch { $blocked = $true }
    Assert-Test $blocked 'junction ancestor rejected'
    Assert-Test ([IO.File]::Exists($f.Exe)) 'junction probe preserves target'
    [IO.Directory]::Delete($junction, $false)

    # Exercise the real CMD bytes and self-removal in a fixture, never real folders.
    $f = New-TestFixture
    $corePath = (Join-Path $repository 'native\Uninstall-LostEye.ps1').Replace("'", "''")
    $fixtureLocal = $f.Context.LocalAppData.Replace("'", "''")
    $fixturePrograms = $f.Context.ProgramsDirectory.Replace("'", "''")
    $fixtureRegistry = $f.Root.Name.Substring('HKEY_CURRENT_USER\'.Length).Replace("'", "''")
    $wrapper = @"
`$ErrorActionPreference = 'Stop'
. '$corePath'
`$fixtureContext = [pscustomobject]@{ LocalAppData = '$fixtureLocal'; ProgramsDirectory = '$fixturePrograms'; RegistryRoot = [Microsoft.Win32.Registry]::CurrentUser.OpenSubKey('$fixtureRegistry',`$true); SourceDirectory = `$PSScriptRoot; ProcessProbe = { @() } }
Set-Location -LiteralPath ([IO.Path]::GetTempPath())
try { [void](Invoke-LostEyeUninstall `$fixtureContext); exit 0 } catch { Write-Error `$_; exit 1 }
"@
    [IO.File]::WriteAllText((Join-Path $f.Directory 'Uninstall-LostEye.ps1'), $wrapper)
    $cmd = Join-Path $f.Directory 'Uninstaller.cmd'
    $selfRemoveOutput = & $env:ComSpec /d /c "call `"$cmd`" < nul" 2>&1
    if ($LASTEXITCODE -ne 0) { throw "CMD fixture failed: $selfRemoveOutput" }
    Assert-Test (![IO.Directory]::Exists($f.Directory)) 'real CMD handles ! in paths and removes both active scripts and folder'
    $f = New-TestFixture
    [IO.File]::WriteAllText((Join-Path $f.Directory 'Uninstall-LostEye.ps1'), 'exit 17')
    $cmd = Join-Path $f.Directory 'Uninstaller.cmd'
    & $env:ComSpec /d /c "call `"$cmd`" < nul"
    Assert-Test ($LASTEXITCODE -eq 1) 'CMD reports PowerShell failure with exit code 1'
    Write-Output "Passed $script:checks checks. Fixtures preserved at $testRoot"
    Write-Output "Isolated registry fixture: HKCU\$registryPath"
} finally { $sandbox.Dispose() }

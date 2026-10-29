param([switch]$Quiet)
$ErrorActionPreference = 'Stop'
Import-Module (Join-Path $PSHOME 'Modules\Microsoft.PowerShell.Utility\Microsoft.PowerShell.Utility.psd1') -ErrorAction Stop
$script:LostEyeUninstallSourceDirectory = $PSScriptRoot

function Assert-LostEyePlainPath([string]$Path) {
    $current = [IO.Path]::GetFullPath($Path)
    while ($current) {
        try {
            $attributes = [IO.File]::GetAttributes($current)
            if (($attributes -band [IO.FileAttributes]::ReparsePoint) -ne 0) { throw "A link or junction was found at '$current'. Nothing will be removed through it." }
        } catch [IO.FileNotFoundException] { } catch [IO.DirectoryNotFoundException] { }
        $parent = [IO.Path]::GetDirectoryName($current.TrimEnd('\'))
        if (!$parent -or $parent -eq $current) { break }
        $current = $parent
    }
}

function Get-LostEyeRunningPaths {
    foreach ($process in [Diagnostics.Process]::GetProcessesByName('Lost Eye')) {
        try { $process.MainModule.FileName }
        catch { throw 'Close all Lost Eye windows, then run Uninstaller.cmd again.' }
        finally { $process.Dispose() }
    }
}

function New-LostEyeUninstallContext {
    [pscustomobject]@{
        LocalAppData = [Environment]::GetFolderPath('LocalApplicationData')
        ProgramsDirectory = [Environment]::GetFolderPath('Programs')
        RegistryRoot = [Microsoft.Win32.Registry]::CurrentUser
        SourceDirectory = $script:LostEyeUninstallSourceDirectory
        ProcessProbe = { Get-LostEyeRunningPaths }
    }
}

function Test-LostEyeRegistryValue($Root, [string]$Key, [string]$Name, $Value, [Microsoft.Win32.RegistryValueKind]$Kind) {
    $handle = $Root.OpenSubKey($Key)
    if (!$handle) { return $false }
    try {
        if ($handle.GetValueNames() -notcontains $Name -or $handle.GetValueKind($Name) -ne $Kind) { return $false }
        $actual = $handle.GetValue($Name, $null, [Microsoft.Win32.RegistryValueOptions]::DoNotExpandEnvironmentNames)
        if ($Kind -eq [Microsoft.Win32.RegistryValueKind]::None) { return @($actual).Count -eq 0 }
        return [string]::Equals([string]$actual, [string]$Value, [StringComparison]::OrdinalIgnoreCase)
    } finally { $handle.Dispose() }
}

function Add-LostEyeRegistryRemoval($Plan, $Root, [string]$Key, [string]$Name, $Value, [Microsoft.Win32.RegistryValueKind]$Kind = [Microsoft.Win32.RegistryValueKind]::String) {
    if (Test-LostEyeRegistryValue $Root $Key $Name $Value $Kind) {
        $Plan.Add([pscustomobject]@{ Key = $Key; Name = $Name; Value = $Value; Kind = $Kind })
    }
}

function Get-LostEyeRegistryPlan($Root, [string]$Executable) {
    $plan = [Collections.Generic.List[object]]::new()
    $app = 'Software\Classes\Applications\Lost Eye.exe'
    $prog = 'Software\Classes\LostEye.Video'
    $caps = 'Software\Lost Eye\Capabilities'
    $command = '"' + $Executable + '" "%1"'
    $icon = '"' + $Executable + '",0'
    $extensions = @('.mp4','.m4v','.mov','.mkv','.webm','.avi','.wmv','.mpg','.mpeg','.ts','.m2ts','.3gp')
    if (Test-LostEyeRegistryValue $Root "$app\shell\open\command" '' $command String) {
        Add-LostEyeRegistryRemoval $plan $Root $app 'FriendlyAppName' 'Lost Eye'
        Add-LostEyeRegistryRemoval $plan $Root $app 'MultiSelectModel' 'Single'
        Add-LostEyeRegistryRemoval $plan $Root "$app\DefaultIcon" '' $icon
        foreach ($ext in $extensions) { Add-LostEyeRegistryRemoval $plan $Root "$app\SupportedTypes" $ext '' }
        Add-LostEyeRegistryRemoval $plan $Root "$app\shell\open\command" '' $command
    }
    if (Test-LostEyeRegistryValue $Root "$prog\shell\open\command" '' $command String) {
        Add-LostEyeRegistryRemoval $plan $Root $prog '' 'Lost Eye video'
        Add-LostEyeRegistryRemoval $plan $Root "$prog\DefaultIcon" '' $icon
        foreach ($ext in $extensions) { Add-LostEyeRegistryRemoval $plan $Root "Software\Classes\$ext\OpenWithProgids" 'LostEye.Video' ([byte[]]@()) None }
        Add-LostEyeRegistryRemoval $plan $Root "$prog\shell\open\command" '' $command
    }
    if (Test-LostEyeRegistryValue $Root $caps 'ApplicationIcon' $icon String) {
        Add-LostEyeRegistryRemoval $plan $Root $caps 'ApplicationName' 'Lost Eye'
        Add-LostEyeRegistryRemoval $plan $Root $caps 'ApplicationDescription' 'A simple video player'
        Add-LostEyeRegistryRemoval $plan $Root $caps 'ApplicationIcon' $icon
        foreach ($ext in $extensions) { Add-LostEyeRegistryRemoval $plan $Root "$caps\FileAssociations" $ext 'LostEye.Video' }
        Add-LostEyeRegistryRemoval $plan $Root 'Software\RegisteredApplications' 'Lost Eye' $caps
    }
    Add-LostEyeRegistryRemoval $plan $Root 'Software\Microsoft\Windows\CurrentVersion\App Paths\Lost Eye.exe' '' $Executable
    return ,$plan
}

function Get-LostEyeShortcutTarget([string]$Path) {
    $shell = New-Object -ComObject WScript.Shell
    try { $shortcut = $shell.CreateShortcut($Path); return $shortcut.TargetPath }
    finally {
        if ($shortcut) { [void][Runtime.InteropServices.Marshal]::FinalReleaseComObject($shortcut) }
        [void][Runtime.InteropServices.Marshal]::FinalReleaseComObject($shell)
    }
}

function Send-LostEyeAssociationChange {
    if (!('LostEyeUninstallNative' -as [type])) {
        Add-Type -TypeDefinition @'
using System;
using System.Runtime.InteropServices;
public static class LostEyeUninstallNative {
    [DllImport("shell32.dll")]
    public static extern void SHChangeNotify(uint eventId, uint flags, IntPtr item1, IntPtr item2);
}
'@
    }
    [LostEyeUninstallNative]::SHChangeNotify(0x08000000, 0, [IntPtr]::Zero, [IntPtr]::Zero)
}

function Remove-LostEyeEmptyRegistryKeys($Root, $RemovedValues) {
    $boundaries = @('Software\Classes\Applications\Lost Eye.exe','Software\Classes\LostEye.Video','Software\Lost Eye\Capabilities','Software\Microsoft\Windows\CurrentVersion\App Paths\Lost Eye.exe')
    $keys = [Collections.Generic.HashSet[string]]::new([StringComparer]::OrdinalIgnoreCase)
    foreach ($entry in $RemovedValues) {
        foreach ($boundary in $boundaries) {
            if ($entry.Key -eq $boundary -or $entry.Key.StartsWith($boundary + '\', [StringComparison]::OrdinalIgnoreCase)) {
                $path = $entry.Key
                while ($path.Length -ge $boundary.Length) {
                    [void]$keys.Add($path)
                    if ($path -eq $boundary) { break }
                    $path = $path.Substring(0, $path.LastIndexOf('\'))
                }
            }
        }
        if ($entry.Key -match '^Software\\Classes\\\.[a-z0-9]+\\OpenWithProgids$') { [void]$keys.Add($entry.Key) }
    }
    foreach ($path in ($keys | Sort-Object Length -Descending)) {
        try {
            $key = $Root.OpenSubKey($path)
            if (!$key) { continue }
            try { $empty = $key.ValueCount -eq 0 -and $key.SubKeyCount -eq 0 } finally { $key.Dispose() }
            if ($empty) { $Root.DeleteSubKey($path, $false) }
        } catch { # A changed or inaccessible key is left in place.
        }
    }
}

function Invoke-LostEyeUninstall($Context) {
    # Production uses known folders. Tests may inject only a dedicated HKCU sandbox
    # and a temporary filesystem tree; these are not command-line parameters.
    $registryName = $Context.RegistryRoot.Name
    $localRoot = [IO.Path]::GetFullPath($Context.LocalAppData).TrimEnd('\')
    $programs = [IO.Path]::GetFullPath($Context.ProgramsDirectory).TrimEnd('\')
    if ($registryName -eq 'HKEY_CURRENT_USER') {
        if ($localRoot -ne [Environment]::GetFolderPath('LocalApplicationData').TrimEnd('\') -or $programs -ne [Environment]::GetFolderPath('Programs').TrimEnd('\')) { throw 'Unexpected user installation location.' }
    } elseif ($registryName -like 'HKEY_CURRENT_USER\Software\LostEye.Uninstall.Tests.*') {
        $tempPrefix = [IO.Path]::GetFullPath([IO.Path]::GetTempPath()).TrimEnd('\') + '\'
        if (!$localRoot.StartsWith($tempPrefix, [StringComparison]::OrdinalIgnoreCase) -or !$programs.StartsWith($tempPrefix, [StringComparison]::OrdinalIgnoreCase)) { throw 'Test locations must stay inside the temporary folder.' }
    } else { throw 'Only current-user Lost Eye installation data can be removed.' }
    $directory = Join-Path $localRoot 'Programs\Lost Eye\0.3.2'
    $executable = Join-Path $directory 'Lost Eye.exe'
    $shortcut = Join-Path $programs 'Lost Eye.lnk'
    $backup = Join-Path $directory 'Previous Lost Eye shortcut.lnk'
    $files = @($executable, (Join-Path $directory 'Uninstaller.cmd'), (Join-Path $directory 'Uninstall-LostEye.ps1'))
    foreach ($path in @($directory, $shortcut, $backup) + $files) { Assert-LostEyePlainPath $path }
    if ([IO.Directory]::Exists($executable)) { throw 'The installed player path is a directory. Nothing was removed.' }
    foreach ($runningPath in @(& $Context.ProcessProbe)) {
        if ([string]::Equals($runningPath, $executable, [StringComparison]::OrdinalIgnoreCase)) { throw 'Close Lost Eye, then run Uninstaller.cmd again. The player was not stopped.' }
    }
    if ([IO.File]::Exists($executable)) {
        $identity = [Diagnostics.FileVersionInfo]::GetVersionInfo($executable)
        if ($identity.ProductName -ne 'Lost Eye' -or $identity.OriginalFilename -ne 'Lost Eye.exe' -or $identity.FileMajorPart -ne 0 -or $identity.FileMinorPart -ne 3 -or $identity.FileBuildPart -ne 2) { throw 'The installed executable is not the expected Lost Eye 0.3.2 file. Nothing was removed.' }
    }
    foreach ($name in @('Uninstaller.cmd','Uninstall-LostEye.ps1')) {
        $installed = Join-Path $directory $name
        if ([IO.Directory]::Exists($installed)) { throw "The installed '$name' path is a directory. Nothing was removed." }
        if ([IO.File]::Exists($installed)) {
            $source = Join-Path $Context.SourceDirectory $name
            if (!(Test-Path -LiteralPath $source -PathType Leaf) -or (Get-FileHash -LiteralPath $source).Hash -ne (Get-FileHash -LiteralPath $installed).Hash) { throw "The installed '$name' differs from this uninstaller. Nothing was removed." }
        }
    }
    $registryPlan = Get-LostEyeRegistryPlan $Context.RegistryRoot $executable
    $deletedValues = [Collections.Generic.List[object]]::new()
    $savedFiles = @{}
    foreach ($path in $files) { if ([IO.File]::Exists($path)) { $savedFiles[$path] = [IO.File]::ReadAllBytes($path) } }
    $shortcutBefore = $null
    $shortcutAfter = $null
    $shortcutChanged = $false
    $backupRestored = $false
    $backupBytes = $null
    if ([IO.File]::Exists($shortcut) -and (Get-LostEyeShortcutTarget $shortcut) -eq $executable) {
        $shortcutBefore = [IO.File]::ReadAllBytes($shortcut)
        if ([IO.File]::Exists($backup)) {
            $previousTarget = Get-LostEyeShortcutTarget $backup
            if ($previousTarget -and $previousTarget -ne $executable -and [IO.File]::Exists($previousTarget)) {
                $shortcutAfter = [IO.File]::ReadAllBytes($backup)
                $backupBytes = $shortcutAfter
            }
        }
    }
    try {
        # Remove the executable before unregistering, so a locked player cannot
        # leave a still-installed player with its registration removed.
        if ([IO.File]::Exists($executable)) { [IO.File]::Delete($executable) }
        foreach ($entry in $registryPlan) {
            if (Test-LostEyeRegistryValue $Context.RegistryRoot $entry.Key $entry.Name $entry.Value $entry.Kind) {
                $key = $Context.RegistryRoot.OpenSubKey($entry.Key, $true)
                try { $key.DeleteValue($entry.Name, $false); $deletedValues.Add($entry) } finally { $key.Dispose() }
            }
        }
        if ($null -ne $shortcutBefore -and [IO.File]::Exists($shortcut) -and (Get-LostEyeShortcutTarget $shortcut) -eq $executable) {
            if ($null -ne $shortcutAfter) { [IO.File]::WriteAllBytes($shortcut, $shortcutAfter) }
            else { [IO.File]::Delete($shortcut) }
            $shortcutChanged = $true
            # The installer may have found this backup already present. Keep it
            # after restoration because ownership of that earlier file is unknown.
            if ($null -ne $shortcutAfter) { $backupRestored = $true }
        }
        foreach ($path in $files | Where-Object { $_ -ne $executable }) { if ([IO.File]::Exists($path)) { [IO.File]::Delete($path) } }
    } catch {
        $failure = $_.Exception.Message
        $rollbackErrors = [Collections.Generic.List[string]]::new()
        foreach ($path in $savedFiles.Keys) {
            try { if (![IO.File]::Exists($path)) { [IO.File]::WriteAllBytes($path, $savedFiles[$path]) } } catch { $rollbackErrors.Add($_.Exception.Message) }
        }
        foreach ($entry in $deletedValues) {
            try {
                $key = $Context.RegistryRoot.CreateSubKey($entry.Key)
                try { if ($key.GetValueNames() -notcontains $entry.Name) { $key.SetValue($entry.Name, $entry.Value, $entry.Kind) } } finally { $key.Dispose() }
            } catch { $rollbackErrors.Add($_.Exception.Message) }
        }
        try {
            if ($shortcutChanged -and (![IO.File]::Exists($shortcut) -or ($null -ne $shortcutAfter -and (Get-LostEyeShortcutTarget $shortcut) -eq $previousTarget))) { [IO.File]::WriteAllBytes($shortcut, $shortcutBefore) }
            if ($backupRestored -and ![IO.File]::Exists($backup)) { [IO.File]::WriteAllBytes($backup, $backupBytes) }
        } catch { $rollbackErrors.Add($_.Exception.Message) }
        if ($rollbackErrors.Count) { throw "Uninstall could not finish: $failure. Some rollback steps also failed: $($rollbackErrors -join '; '). Keep this folder and try again after closing Lost Eye." }
        throw "Uninstall could not finish: $failure. Removed installation data was restored. Close Lost Eye and try again."
    }
    Remove-LostEyeEmptyRegistryKeys $Context.RegistryRoot $deletedValues
    $remaining = @()
    if ([IO.Directory]::Exists($directory)) {
        $remaining = @([IO.Directory]::EnumerateFileSystemEntries($directory))
        if ($remaining.Count -eq 0) { [IO.Directory]::Delete($directory, $false) }
    }
    [pscustomobject]@{ Removed = $true; RemainingFiles = $remaining; RestoredPreviousShortcut = $backupRestored }
}

if ($MyInvocation.InvocationName -ne '.') {
    $resultCode = 0
    try {
        Set-Location -LiteralPath ([IO.Path]::GetTempPath())
        $result = Invoke-LostEyeUninstall (New-LostEyeUninstallContext)
        Send-LostEyeAssociationChange
        Write-Host 'Lost Eye 0.3.2 has been uninstalled for this Windows account.'
        if ($result.RemainingFiles.Count) { Write-Host 'Other files or a preserved shortcut backup remain in the installation folder.' }
        Write-Host 'Other versions, your videos, and default-player choices were left unchanged.'
    } catch { $resultCode = 1; Write-Host $_.Exception.Message -ForegroundColor Red }
    if (!$Quiet) { [void](Read-Host 'Press Enter to close') }
    exit $resultCode
}

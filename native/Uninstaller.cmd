@echo off
setlocal DisableDelayedExpansion
set "lostEyeUninstallScript=%~dp0Uninstall-LostEye.ps1"
cd /d "%TEMP%"
(
  "%SystemRoot%\System32\WindowsPowerShell\v1.0\powershell.exe" -NoProfile -ExecutionPolicy RemoteSigned -File "%lostEyeUninstallScript%" -Quiet
  if errorlevel 1 (
    echo.
    pause
    exit 1
  ) else (
    echo.
    pause
    exit 0
  )
)

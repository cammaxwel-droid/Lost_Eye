@echo off
setlocal
echo Building Lost Eye from the source in this folder...
powershell.exe -NoProfile -File "%~dp0scripts\Build-Native.ps1"
set "lostEyeBuildResult=%ERRORLEVEL%"
if not "%lostEyeBuildResult%"=="0" (
  echo.
  echo Build failed. Read the error above and BUILDING.md in this folder.
) else (
  echo.
  echo Build complete. The launch and optional setup paths are shown above.
)
pause
exit /b %lostEyeBuildResult%

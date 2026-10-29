# Build Lost Eye from source

This builds the native Windows version from the included C++ source and duck icon. It uses Windows Media Foundation and Direct3D 11. The build uses the installed Microsoft toolchain and Windows SDK without downloading packages or codec libraries. Read [LICENSE](LICENSE) and [LICENSING.md](LICENSING.md) for permission to use this source; personal source builds do not grant commercial redistribution rights.

## One-time prerequisites

Use Windows 11 x64 and an appropriately licensed Microsoft C++ toolchain:

1. For an eligible individual developer, install [Visual Studio Community](https://visualstudio.microsoft.com/downloads/). Check the [Community licence terms](https://visualstudio.microsoft.com/license-terms/) for your circumstances. An appropriately licensed existing Visual Studio or Build Tools installation also works.
2. In Visual Studio Installer, select **Desktop development with C++**. Keep the **MSVC x64/x86 build tools** and a **Windows 11 SDK** selected. See Microsoft's [C++ installation instructions](https://learn.microsoft.com/en-us/cpp/overview/acquire-msvc).
3. Complete installation before building. The script finds the installed tools automatically; a developer command prompt is unnecessary.

Microsoft's tools and Windows SDK have their own licence terms. Standalone Build Tools are not an unrestricted substitute for a Visual Studio licence for every use; see the [Visual Studio 2026 Build Tools terms](https://visualstudio.microsoft.com/license-terms/vs2026-ga-diagnostic-buildtools/). The build scripts do not install prerequisites or change their licensing. Git, Python, CMake, .NET SDK, and a separate media player are not required for this native build.

## Build and launch

1. Extract the source ZIP into a normal writable folder. If using the portable package, first extract its `Lost-Eye-native-source.zip`. Do not run the build from inside a ZIP preview.
2. Double-click **Build Lost Eye.cmd**. Give the compiler time to finish; the window stays open and reports any error.
3. When it says **Built**, open the reported `Lost Eye.exe`. The first build normally creates:

   `artifacts\Lost-Eye-0.3.2-win-x64\Lost Eye.exe`

   Repeating a build adds a date and unique suffix to the folder name to preserve your earlier build. Close any running Lost Eye before trying another build.
4. Choose **Open file**, drag a video onto the window, or pass a video path to the executable. Use a video already known to play on that Windows installation. Supported formats depend on Windows components and codecs installed on the computer.

The executable is portable and needs no developer tools to run. It is currently unsigned. If Windows blocks downloaded files, review their source before deciding to allow them. For a trusted downloaded ZIP, its **Properties > Unblock** option, when present, can remove the download mark before extracting it again. If PowerShell reports that script execution is disabled, follow your computer's execution-policy requirements or contact its administrator. The launcher does not change or bypass those policies.

## Optional installation and Open with

Double-click **Install Lost Eye.cmd** beside the newly built executable. It runs that executable's built-in `--install` command, copies it and its embedded uninstall scripts to `%LOCALAPPDATA%\Programs\Lost Eye\0.3.2`, and adds Start and **Open with** entries for your Windows account. It does not change your default video player and does not need administrator access.

Then right-click a video, choose **Open with > Choose another app**, and select Lost Eye. The installer preserves a different existing executable in the same version's installation folder and reports that conflict instead of overwriting it. You can still run the newly built executable directly from its output folder. A rebuild with changed source is not automatically a new application version.

To uninstall 0.3.2, close the player and run **Uninstaller.cmd** from the installed folder or extracted portable package, keeping **Uninstall-LostEye.ps1** beside it. It removes only the installed version and entries still owned by it. Videos, default-player choices, downloads, source, older versions and unknown files are preserved. If Lost Eye was your chosen default, select a replacement in Windows afterward.

The uninstaller uses Windows PowerShell already supplied by Windows. Its launcher requests RemoteSigned for that process only; it does not change the saved execution policy or override Group Policy. For trusted downloaded files, unblock the ZIP before extracting if required by that policy.

## Build a portable package

For a deliberately named build, open PowerShell in the extracted source folder and run these commands one at a time:

```powershell
.\scripts\Build-Native.ps1 -OutputName 'Lost-Eye-my-build'
.\scripts\Package-Native.ps1 -OutputName 'Lost-Eye-my-build'
```

The result is `artifacts\Lost-Eye-my-build.zip`, containing `Lost Eye.exe`, install/uninstall launchers and the uninstall script, documentation, licence and third-party notices, and matching native source in `Lost-Eye-native-source.zip`. `BUILD-INFO.json` records compiler/SDK versions and source/executable hashes; `SHA256SUMS.txt` lists the packaged files, excluding itself. A `.zip.sha256` file beside the ZIP records the archive hash. The package contains only the current native implementation and its listed files. Keep your use within [LICENSE](LICENSE).

Explicit output names never overwrite an existing build or ZIP. Choose a new name for another attempt. Packaging checks that the compiled source, icon, manifest, build script, and executable still match the recorded build. If they changed, rebuild under a new name first. Packaging uses a fresh staging folder rather than collecting unrelated files from a previous build.

## Developer verification

Normal builds compile only the player. Add `-WithTests` to compile the test executable as well, or use `-TestsOnly` for tests alone:

```powershell
.\scripts\Build-Native.ps1 -OutputName 'Lost-Eye-check' -WithTests
```

Build intermediates and dependency inspection go into `artifacts\Lost-Eye-check-build`; the test executable goes into `artifacts\Lost-Eye-check-tests`. Compiling tests does not run them. Test fixtures and Windows playback verification are separate; see root `VERIFICATION.md` for this version's verification. Historical reports remain in the repository's `legacy/` archive.

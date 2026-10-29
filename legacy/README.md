# Lost Eye

This directory preserves the older 0.1.0 implementation, its source, assets, build tools and original dependency notices. It is separate from the current native player described in the repository's root README. The additional `native-0.3.0-*` records preserve historical native verification and notices; they do not describe this 0.1.0 player.

All paths and commands below are relative to this directory (or an extracted legacy source archive). In the repository, open PowerShell in `legacy` first. Builds and tests write into its own `artifacts` directory. The `LICENSE` here and the notices in `licenses/` retain the earlier terms.

A small Windows 11 x64 video player. Open a video and watch it immediately.

## Run the portable version

Extract the entire `Lost-Eye-0.1.0-win-x64-r2.zip` into a folder, then double-click `Lost Eye.exe`. Keep the dependencies beside it. No .NET, VLC installation, developer tools, administrator access, or account is needed.

Open a video with **Open file**, drag one video onto the player, or use Windows **Open with > Choose another app > Choose an app on your PC** and select `Lost Eye.exe`. Choose **Just once** to leave your default player unchanged. If you want double-clicks to open Lost Eye, explicitly choose it as your default in Windows yourself.

Optionally run `Register Open With.cmd` to add Lost Eye to the current user's Open with choices. This never changes defaults. Keep the folder in place; rerun registration after moving it. `Remove Open With.cmd` removes this folder's optional listing. Neither script needs administrator rights. Ordinary launches write no associations.

## Controls

- **Play / Pause / Replay**, seek bar, and elapsed / total time.
- **Volume** slider and **Mute / Unmute**.
- **Full screen**, F11, or F; Escape returns to the window.
- **Loop** repeats the entire video until switched off. It has no A-B markers.
- Ctrl+O opens a file. M toggles mute; L toggles loop. Left/right seek five seconds when a slider is not focused. Space plays/pauses when focus is in the video area; focused buttons retain normal keyboard behavior.

Missing, empty, damaged, and unsupported files show a message inside the player. Open another file to recover. Only one file is opened at a time; there is no playlist, recent-file history, settings page, account, ad, subscription, telemetry, or update service.

## First test on your laptop

1. Open a familiar local MP4. Confirm moving video and audible, synchronized sound.
2. Pause, drag the seek bar, resume, adjust volume, then mute/unmute.
3. Enable Loop and let the clip finish twice. Disable Loop and confirm it stops at the next end.
4. Enter/exit fullscreen and resize the window. Check the video keeps its aspect ratio.
5. Drop another file onto the video area while playing. Try MKV/WebM if available.
6. Test Explorer Open with using a path containing spaces. Use Just once to preserve your default.
7. Try a damaged or unsupported file, then reopen a valid video.

This first version is unsigned. Windows hardware, drivers, codecs, HDR, protected media, and unusual/damaged files can vary. It does not promise every codec or machine. Large/slow-storage files may need more time. This build targets x64, not native ARM64 or 32-bit Windows.

## Build from source

Developers need the .NET 10 SDK on Windows. End users do not.

```powershell
.\scripts\Build-Portable.ps1
```

The script restores pinned packages, publishes a self-contained x64 folder, copies licenses and source references, creates a SHA-256 inventory, and writes the portable ZIP. It refuses to overwrite an existing release folder/ZIP. Choose another `-OutputName` for a new build.

`src/LostEye`: Windows Forms UI, file/error handling, LibVLCSharp integration, and optional association registration. `tests/LostEye.SmokeTests`: real decoding and state checks against disposable fixtures. `scripts/Test.ps1` creates test clips using FFmpeg (development only) and runs the checks. The generated duck and actual 7-size ICO are in `assets`; `scripts/Convert-Icon.ps1` reproducibly packages that PNG without replacing the original.

## Engine and redistribution

.NET Windows Forms keeps the native desktop UI straightforward. LibVLC 3.0.24 with LibVLCSharp 3.10.2 provides mature codecs and automatic hardware decoding. Managed/runtime version: .NET 10.0.14. Native libraries remain separate and replaceable.

Lost Eye code is licensed GPL-2.0-or-later because the supplied native plugin set includes GPL components. See `LICENSE`, `THIRD-PARTY-NOTICES.md`, `SOURCES.md`, and `licenses/`. Source for Lost Eye and the two main VideoLAN components accompanies the portable package. Preserve the notices and source access when redistributing. There is no VideoLAN endorsement; the generated yellow duck is original artwork for this project.

See `VERIFICATION.md` for actual checks and remaining manual tests.

The build generates a relocatable VLC plugin cache to avoid a slow first-launch scan. If replacing native VLC components, regenerate `libvlc/win-x64/plugins/plugins.dat` using the included source tool: `dotnet run --project tools/PluginCache -- "<portable folder>/libvlc/win-x64"`. Removing the cache allows runtime discovery again, but the first scan can be slow. The tool is for developers and is not needed to run the portable app.

# Lost Eye 0.3.1

A compact Windows 11 x64 video player with the original yellow duck icon.

This is now a native C++ application using Windows Media Foundation and Direct3D 11. It uses Windows codecs and the graphics device where supported. It does not use or launch VLC, Windows Media Player, another player app, .NET, or a third-party codec runtime. Only Windows system DLLs are imported; the C++ runtime is linked into the EXE.

## Run

Run **Lost Eye.exe**. It is a self-contained executable: keep or move it wherever you want. Open a video with **Open file**, drop a video onto the window, or pass its path on the command line.

For convenient Windows integration, run **Install Lost Eye.cmd** once. It copies the EXE to `%LOCALAPPDATA%\Programs\Lost Eye\0.3.1`, adds Lost Eye to Start, and registers it in **Open with**. It requires no administrator rights and does not change your default player. Older version folders are preserved. Existing different files in the same version folder are not overwritten.

Right-click a video > **Open with > Lost Eye**. If you want double-clicking videos to use Lost Eye, choose **Choose another app > Lost Eye > Always** yourself. Use **Just once** to leave defaults unchanged.

## Controls

Back to start, rewind 10 seconds, play/pause/replay, forward 10 seconds, skip to end, seek, elapsed/total time, volume/mute, fullscreen, and one whole-video Loop checkbox. Ctrl+O opens a file, F11 enters/exits fullscreen, Escape exits fullscreen, M toggles mute, and L toggles loop. Left/right seek 10 seconds, Home returns to the start, and End skips to the end when a slider isn't focused. Double-click the video to toggle fullscreen.

The window appears before the engine initializes. Opening another video reuses the existing window. Missing, empty, damaged or unavailable formats show a message and let you choose another file.

Format support depends on the codecs provided by Windows and the hardware/driver. Tested locally: MP4/H.264/AAC, MKV/H.264/AAC, and WebM/VP9/Opus. This does not guarantee every codec, HDR, protected file or graphics device. Windows N editions without media components need those Windows components; no player app or third-party codec library is bundled.

## Source and build

If you downloaded source rather than a portable build, it does not contain a ready-to-run EXE. Extract the download, install an eligible Visual Studio Community edition with **Desktop development with C++** (or use another appropriately licensed Microsoft toolchain), then double-click **Build Lost Eye.cmd**. See [BUILDING.md](BUILDING.md) for exact prerequisites, output location and troubleshooting.

The active implementation is `native/LostEye.cpp`, with `native/LostEye.rc` and `native/app.manifest`. The equivalent PowerShell command is:

```powershell
.\scripts\Build-Native.ps1
```

Developer tools are needed only to build source. The executable needs none of them.

The native portable ZIP contains one app EXE, an optional per-user setup launcher, documentation, build records, and matching native source archive. Extract it and run the EXE without installing developer tools. It contains no VLC or .NET files. Windows supplies the media components; Microsoft runtime code is linked into the EXE.

The original .NET/LibVLC code in `src/LostEye` and its scripts remain in the repository for legacy use. Their documentation is in `legacy/`; do not run `Build-Portable.ps1` to build the native player. Existing local older packages are preserved.

## License

Native 0.3.1 is source-available for personal, noncommercial source builds and use under [LICENSE](LICENSE). Commercial rights are reserved to the extent held by the owner; commercial use or redistribution needs separate permission. Third-party terms still apply. Previously supplied GPL copies, including native 0.3.0 and its artwork, retain those permissions. See [LICENSING.md](LICENSING.md) and [THIRD-PARTY-NOTICES.md](THIRD-PARTY-NOTICES.md). Original duck assets and the generation record remain unchanged.

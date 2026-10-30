# Lost Eye 0.3.3 candidate

A compact Windows 11 x64 video player with the original yellow duck icon.

This is a review candidate. The published 0.3.2 release remains unchanged. Extract `Lost-Eye-0.3.3-candidate-win-x64.zip` and run **Lost Eye.exe** to try it without installing it.

Lost Eye uses the computer's graphics device through Direct3D 11, with Windows Media Foundation providing video decoding and audio. It requests hardware acceleration and shares the graphics device with the media engine. Windows may decode unsupported formats in software; if a hardware graphics device cannot be created, the app can use software rendering. No separate player installation is required.

## Run

Run **Lost Eye.exe**. It is a self-contained executable: keep or move it wherever you want. Open a video with **Open file**, drop a video onto the window, or pass its path on the command line.

For convenient Windows integration, run **Install Lost Eye.cmd** once. It copies the EXE and uninstall scripts to `%LOCALAPPDATA%\Programs\Lost Eye\0.3.3`, adds Lost Eye to Start, and registers it in **Open with**. It requires no administrator rights and does not change your default player. Older version folders are preserved. Existing different files in the same version folder are not overwritten.

Right-click a video > **Open with > Lost Eye**. If you want double-clicking videos to use Lost Eye, choose **Choose another app > Lost Eye > Always** yourself. Use **Just once** to leave defaults unchanged.

## Uninstall

Close Lost Eye, then double-click **Uninstaller.cmd** in the extracted portable folder or `%LOCALAPPDATA%\Programs\Lost Eye\0.3.3`. Keep its companion **Uninstall-LostEye.ps1** beside it. The installed EXE also contains both scripts, so running its setup does not depend on retaining the download folder.

The uninstaller removes the installed 0.3.3 copy and Windows entries that still belong to that copy. It preserves videos, source/download folders, older versions, unknown files and entries belonging to another installation. Your default-player choices are not rewritten. If you previously chose Lost Eye as a default, choose a replacement in Windows after uninstalling. If a previous Start shortcut was backed up, it is restored when safe.

Uninstall needs no administrator rights. It reports a running player or files it cannot safely remove instead of forcing deletion. A portable-only copy can simply be removed from its extracted folder when you no longer want it.

## Controls

**Back** opens the previous video and **Skip** opens the next video in the same folder, ordered naturally by filename (for example, video2 before video10). **Left/Right Arrow** do the same, regardless of which player control has focus. Navigation stays in that folder, ignores subfolders and file links, and stops at the first or last video without wrapping. Opening a video in another folder explicitly starts navigation in that folder.

**Space** always toggles play/pause, regardless of the last clicked control. Clicking the video area also toggles play/pause. Double-clicking the video does not switch fullscreen; use **Full screen** or **F11**. **Escape** exits fullscreen.

**Rewind 10s** and **Forward 10s** seek within the current video. The seek bar and **Home/End** move within that video too. **Ctrl+O** opens a file, **M** toggles mute, and **L** toggles the whole-video Loop checkbox.

Scroll the mouse wheel up/down anywhere over the player to raise/lower volume by five percentage points per notch. Volume stays between 0% and 100%; an adjustment unmutes the player. The volume slider and mute button remain available.

The player stays dark, with light text for **Loop** and the other controls. There is no light-mode option.

The window appears before the engine initializes. Opening another video reuses the existing window. Missing, empty, damaged or unavailable formats show a message and let you choose another file.

Format support depends on the codecs provided by Windows and the hardware/driver. Playback evidence and its limits are recorded in [VERIFICATION.md](VERIFICATION.md). This does not guarantee every codec, HDR, protected file or graphics device. Windows N editions without media components need those Windows components; no player app or third-party codec library is bundled.

## Source and build

If you downloaded source rather than a portable build, it does not contain a ready-to-run EXE. Extract the download, install an eligible Visual Studio Community edition with **Desktop development with C++** (or use another appropriately licensed Microsoft toolchain), then double-click **Build Lost Eye.cmd**. See [BUILDING.md](BUILDING.md) for exact prerequisites, output location and troubleshooting.

The active implementation is `native/LostEye.cpp`, with `native/LostEye.rc` and `native/app.manifest`. The equivalent PowerShell command is:

```powershell
.\scripts\Build-Native.ps1
```

Developer tools are needed only to build source. The executable needs none of them.

The native portable ZIP contains one app EXE, setup/uninstall launchers, documentation, build records, and matching native source archive. Extract it and run the EXE without installing developer tools. Windows supplies the media components; Microsoft runtime code is linked into the EXE.

Earlier implementations, build tools and notices are preserved together in the repository's `legacy/` directory. Current source exports and native packages omit that archive. Use the root **Build Lost Eye.cmd** for the current player.

## License

Native 0.3.3 candidate is source-available for personal, noncommercial source builds and use under [LICENSE](LICENSE). Commercial rights are reserved to the extent held by the owner; commercial use or redistribution needs separate permission. See [LICENSING.md](LICENSING.md) for earlier release rights and [THIRD-PARTY-NOTICES.md](THIRD-PARTY-NOTICES.md) for the current component inventory. Original duck assets and the generation record remain unchanged.

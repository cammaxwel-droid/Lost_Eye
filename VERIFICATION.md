# Native Lost Eye 0.3.1 verification

This is the licensing and source-build maintenance revision dated 29 October 2026. Playback code is unchanged from 0.3.0; the only application changes are version resources and the per-user installation folder, now 0.3.1 so existing 0.3.0 files are preserved.

The Windows x64 release and test executable compiled successfully using Microsoft C/C++ 19.51.36260.0, VC tools 14.51.36231 and Windows SDK 10.0.28000.0. Imported DLLs are Windows system components only. Microsoft runtime code is statically linked.

## Checks completed on this Windows laptop

- 38 native 0.3.1 playback checks passed, with zero failures. MP4/H.264/AAC, MKV/H.264/AAC and WebM/VP9/Opus rendered on Intel UHD Graphics 620; 373 frames were presented. Tests covered transport buttons, seeking, pause, volume/mute state, fullscreen/resize, two complete loops, loop-off stopping, replay, repeated opens and invalid files.
- A separate close-during-engine-initialization check passed. Open-with registration checks used an isolated test registry key and preserved its synthetic existing default.
- The native package was extracted, its file checksums checked, and its source ZIP extracted into a fresh directory containing spaces. That source contained no executable or previous build output.
- The actual Build Lost Eye.cmd entry point compiled the player from that extracted source using Windows PowerShell 5.1. The resulting EXE reported version 0.3.1, opened the MP4 path, created a responsive window and closed normally. This checks the launcher entry point programmatically; it is not a claim of manual Explorer double-click testing.
- The first launcher test found mixed PowerShell module-search paths could prevent Get-FileHash loading. The scripts now explicitly import their running host's built-in modules; the fresh source build passed after the fix. Packaging also explicitly imports its host's archive module.
- Legacy notices were compared with the prior repository revision and preserved. The native package inventory contains no VLC, .NET runtime, external codec library or developer tool binaries. Build input and EXE hashes are recorded in BUILD-INFO.json; portable file hashes are in SHA256SUMS.txt.

Local reports are under artifacts/Lost-Eye-0.3.1-source-ready-tests and artifacts/Source Build Check 0.3.1 r2. The final portable EXE is built from the extracted source using the same compiler flags and application code tested by the harness. Build scripts and documentation changed during launcher verification; playback code did not.

Historical native 0.3.0 playback and installation evidence is preserved in native/VERIFICATION.md in the source archive. Legacy .NET/LibVLC evidence is preserved in legacy/VERIFICATION.md in the repository. Those earlier results are not a claim that every later build was tested.

The 0.3.1 install destination was checked in source and version resources; setup was not applied to this account during this maintenance pass. Prior installation evidence remains explicitly scoped to 0.3.0. The legacy .NET/LibVLC packaging script was parsed and reviewed for its corrected notice paths, not rebuilt.

Other machines, fresh Windows installations, Windows N, HDR/protected video, uncommon codecs and unsigned-file prompts are not certified. Visible motion and audible A/V quality still require user acceptance with real videos. Build-tool installation and technical output were verified, not the owner's legal entitlement or commercial chain of title.

# Native Lost Eye 0.3.2 verification

Release verification completed on 30 October 2026. This revision adds the per-user uninstaller and embeds its scripts in the installer. The installation folder is now 0.3.2; older folders are preserved. Playback logic is unchanged from 0.3.1.

The Windows x64 release and test executable compiled successfully using Microsoft C/C++ 19.51.36260.0, VC tools 14.51.36231 and Windows SDK 10.0.28000.0. Imported DLLs are Windows system components only. Microsoft runtime code is statically linked.

## Checks completed on this Windows laptop

- 38 native 0.3.2 playback checks passed, with zero failures. MP4/H.264/AAC, MKV/H.264/AAC and WebM/VP9/Opus rendered on Intel UHD Graphics 620; 375 frames were presented. Tests covered transport buttons, seeking, pause, volume/mute state, fullscreen/resize, two complete loops, loop-off stopping, replay, repeated opens and invalid files.
- 40 uninstall checks passed using the actual 0.3.2 executable in isolated temporary folders and test registry keys. Coverage includes running/locked files, rollback, missing components, repeated uninstall, newer registrations, existing defaults, unknown files, previous shortcut restoration and preservation, reparse points, mismatched portable scripts, and the CMD deleting itself successfully. Paths containing spaces and exclamation marks were exercised.
- Eight additional checks verified a real EXE-only installation followed by running the installed Uninstaller.cmd. Both processes returned zero; uninstall stderr was empty. The installed EXE and embedded scripts matched the release build and were removed. All 12 extension defaults and UserChoice records stayed unchanged. The old Start shortcut was restored byte-for-byte, its backup remained, and the owned Open with command was removed. The account's prior 0.3.0 registration was restored after testing and all four registry exports matched their backups.
- The first integration attempt exposed an exit-code capture problem in the verification harness. An owned System.Diagnostics.Process handle resolved it; the production uninstaller did not need a change.
- An independent source review found no substantive blockers. PowerShell parsing and git diff --check passed. Every build input and the release EXE matched BUILD-INFO.json.
- Five fresh-source checks passed. The actual Build Lost Eye.cmd compiled version 0.3.2 through Windows PowerShell from a fresh 22-file source folder containing spaces, with no prior executable or build output. All seven build inputs matched the release build, and both uninstall resources and copied scripts matched. This source staging preceded the final verification-document update; application/build inputs were unchanged. The fresh build was not separately launched or installed.

Playback results are in artifacts/Lost-Eye-0.3.2-win-x64-tests/results.txt. Installation evidence is in artifacts/installation-verification-r2/. The isolated uninstall suite is scripts/Test-Uninstall.ps1; pass -PlayerExecutable with the built EXE path to test that binary. It uses temporary files and dedicated test registry keys, not the user's installation.

Fresh-source results are in artifacts/Source Build Check 0.3.2 5d26ee69/artifacts/verification/. The release package includes matching native source, BUILD-INFO.json and SHA256SUMS.txt; a separate archive checksum accompanies the download.

Historical playback and installation evidence remains in the repository's legacy/ archive, including native-0.3.0-VERIFICATION.md. These historical results are not claims of testing later versions. Earlier license grants remain in force; archived materials are excluded from the current native package.

## Graphics path and repository cleanup

The cleanup changes documentation and source organization, without changing the 0.3.2 player, icon or uninstall behavior. Native source requests a hardware Direct3D 11 device with video support, shares it with Media Foundation through MF_MEDIA_ENGINE_DXGI_MANAGER, and presents frames through a Direct3D swap chain. Software-device fallback remains available if creating the hardware device fails.

The existing playback checks confirm the Intel UHD Graphics 620 hardware device, decoded pixels in its texture and presented frames. They do not prove that every tested format used a hardware decoder. See Microsoft's [DXGI manager documentation](https://learn.microsoft.com/en-us/windows/win32/medfound/mf-media-engine-dxgi-manager) for the hardware-acceleration interface.

Other machines, fresh Windows installations, Windows N, HDR/protected video, uncommon codecs and unsigned-file prompts are not certified. Visible motion and audible A/V quality still require user acceptance with real videos. Technical output was verified, not the owner's legal entitlement or commercial chain of title.

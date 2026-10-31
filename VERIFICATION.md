# Lost Eye 0.3.3 release verification - 1 November 2026

This final build promotes the reviewed R3 player with unchanged playback/control behavior. Its visible title is Lost Eye 0.3.3, and its window class and single-instance mutex distinguish it from the earlier 0.3.3 candidate. Licence terms, notices, duck artwork and installation overwrite protection are unchanged. The candidate ZIP remains a separate local review artifact.

Fresh checks on this Windows laptop:

- 89 playback/control checks passed, exit 0, with zero failures and 481 presented frames on Intel UHD Graphics 620. MP4, MKV and WebM fixtures rendered, including deterministic first-frame/CANPLAY startup ordering and recovery from damaged files.
- 18 folder-navigation checks passed. Four symbolic-link scenarios were skipped because this account could not create those test links.
- 40 isolated uninstaller checks passed against the final executable. These did not change the real installation or default-player associations.
- MSVC 19.51.36260.0 and Windows SDK 10.0.28000.0 built the player and tests without warnings. Build-input and executable hashes are recorded in BUILD-INFO.json. Imported DLLs are Windows system components only.

Final executable SHA-256: 12066f10926382c1cf0c8509999408bfc4b83624d9f8dc8a6a73d14287912a67.

The portable download is Lost-Eye-0.3.3-win-x64.zip. It contains the executable, optional install/uninstall helpers, required notices, build instructions and matching native source. Lost-Eye-0.3.3-native-source.zip provides the same source separately. SHA-256 sidecars accompany both downloads. Development tools are needed only for source builds.

The runtime test reports remain local under artifacts/release-*-tests.txt; raw diagnostic reports containing machine-specific paths are excluded from published assets. The final source is proposed through a pull request; publication of the tagged release does not merge that PR.

Compatibility is not certified for every PC, codec, driver, Windows edition, HDR/protected video or audio configuration. The installed Start/Open with round trip was not repeated for this final build. Prior installation and candidate evidence below is historical, with its original scope preserved.

---

# R3 portable candidate review - 1 November 2026

This is a local review package, not a published release. It was rebuilt from main snapshot b2f2b89ec295a643f8efbc4a900084756ccf7888 plus the focused startup/test changes included in its matching source ZIP. It has not been installed and no merge or release was performed for this package.

A deterministic regression reproduced a startup race in the previous code: rendering the first frame before the queued CANPLAY event cleared the loading flag, preventing automatic playback. The candidate now keeps the pending start separate from frame rendering. Later CANPLAY events preserve a user's pause.

Fresh Windows checks for this build:

- 89 native playback/control checks passed, exit 0, zero failures, with 481 presented frames on Intel UHD Graphics 620. MP4, MKV and WebM fixtures rendered. The suite covers the startup ordering regression, later-CANPLAY pause preservation, same-folder Back/Skip/Left/Right boundaries, Space with every control focused, video clicks, wheel volume, light Loop text, dark-window mode, seek, fullscreen, loop and damaged-file recovery.
- The input harness now explicitly restores its window before pointer tests and checks usable bounds after fullscreen. This run deliberately started from a minimized launcher and all input checks passed. Earlier runs had seven pointer-related failures; a diagnostic run with normal geometry passed them. The earlier failures and the startup regression's failing baseline are retained with the package's review-checks reports.
- 18 folder navigation checks passed. Four symbolic-link cases remain skipped because Windows did not permit creation of the test links.
- 40 isolated uninstaller checks passed against this build. These use dedicated temporary folders and registry keys; they do not uninstall the user's player or change their default associations.
- The player and test executables compiled without warnings using MSVC 19.51.36260.0 and Windows SDK 10.0.28000.0. Imported DLLs are Windows components only. The executable SHA-256 is c10f817bf850c5c5dd2b3663310bca01ba7829acddb372cf1ad87faf8b4d4ce4.
- An independent review found no blocking issue in the startup fix. The original regression failed on the old logic and passed with this fix. Matching source and executable hashes are recorded in BUILD-INFO.json; the packaged file hashes are in SHA256SUMS.txt.

To review, extract Lost-Eye-0.3.3-CANDIDATE-REVIEW-win-x64.zip and run Lost Eye.exe. The window title is Lost Eye 0.3.3 candidate. No developer tools or separate player are needed to run it. Install Lost Eye.cmd is optional and was not run during this packaging task.

This does not certify every codec, GPU, driver, Windows edition, HDR/protected video or audible A/V quality. Review your own videos on the laptop before approving a release. The installed Start/Open with round trip was not repeated for R3; the installation reports below are historical.

---

# Native Lost Eye 0.3.3 candidate verification

Checked on 30 October 2026. This candidate changes the existing controls: Back/Skip and Left/Right open adjacent videos in the current folder, Space always controls playback, clicking the video toggles playback, and the mouse wheel changes volume. It preserves the dark interface and explicitly draws the Loop label in light text. This is a review candidate, not a published release.

## R2 installation and launch-routing follow-up

The initial candidate was built and packaged but had not replaced the owner's installed player. A fresh inspection found Open with and App Paths pointing to installed 0.3.0, the main Start shortcut pointing to installed 0.3.2, and a duplicate Start shortcut pointing to a downloaded 0.3.2 copy. Earlier players also shared the candidate's single-instance identifiers, allowing an old running window to receive files launched through the candidate.

R2 scopes the single-instance mutex and window class to 0.3.3 and displays **Lost Eye 0.3.3 candidate** in both the empty and video window titles. It does not close older windows. The requested playback controls remain as implemented in the initial candidate.

Checked on the same Windows laptop:

- The rebuilt production EXE and test executable compiled successfully. The installed EXE matches the R2 build record: SHA256 `bf96ae26d9a5b855785d8b9a8f051172af4523d1a94f764f1a37d3a9313c9e95`.
- The actual candidate installer returned zero. Start, both Open with commands and App Paths now resolve to the installed 0.3.3 executable.
- With an old 0.3.0 test window running, the Start shortcut opened a separate candidate window. Windows' enumerated Open with handler then opened the MP4 in that existing candidate process. Its title changed to the requested file; the old window stayed unchanged. The old test window was then closed.
- In the installed production window, Back/Skip and Left/Right selected neighboring fixture files, the last-file boundary stayed in the current folder, a video click resumed playback, Space paused while Full screen had focus without leaving fullscreen, and the wheel changed volume from 80% to 75% and back. A screenshot confirms light Loop text/check mark and the versioned title. UI input checks used an explicitly focused player window.
- The first repeated playback run timed out after a damaged-file recovery check. Its cause was not established. A second run of the same unchanged binary passed all 85 checks, with zero failures and 487 presented frames; MP4, MKV and WebM rendered. Both reports are retained, so the timeout is not represented as a passing run.
- At the owner's request, three old installed versions, an unchanged downloaded 0.3.2 portable folder, nine older build executables, ten verified older portable ZIPs and two obsolete shortcuts were moved to the Recycle Bin (25 targets total). Source, Git history, videos and verification fixtures were preserved. Only 0.3.3 remains in the installation folder. All 12 extension defaults and UserChoice records matched their pre-install snapshots after cleanup.
- An independent review found no new issue in the version-specific handoff or title changes. The earlier full source-build, folder-navigation and isolated uninstall results below are historical evidence for the initial candidate; they were not rerun solely for this routing/title patch.

R2 local evidence is in `artifacts/candidate-install-routing-verification/`, including `installed-proof.json`, `shell-handler-results.txt`, `routing-player-processes.json`, both `playback-*-results.txt` reports, `installed-candidate.png`, `retirement-plan.json`, `retirement-results.txt` and `post-cleanup-results.txt`. The rebuilt package is `Lost-Eye-0.3.3-candidate-r2-win-x64.zip`. It remains a review candidate; no release was published for this follow-up.

## Original candidate checks completed on Windows

- The x64 player and both test executables compiled with Microsoft C/C++ 19.51.36260.0, VC tools 14.51.36231 and Windows SDK 10.0.28000.0. The production EXE is 433,152 bytes. Its imported DLLs are Windows system components; the Microsoft runtime is statically linked. The build record includes all eight application/build inputs, including the new folder-navigation header.
- 85 playback/control checks passed with zero failures and 486 presented frames on Intel UHD Graphics 620. MP4/H.264/AAC, MKV/H.264/AAC and WebM/VP9/Opus fixtures rendered. Coverage includes Space key-down, repeat and key-up with all 13 focus targets; unchanged fullscreen/loop state; video clicks; light Loop text; the dark-window attribute; wheel accumulation, limits and mute state; same-folder Back/Skip/Left/Right; first/last boundaries; recovery from a damaged neighbor; seeking, resize and whole-video loop behavior.
- 18 isolated folder-navigation checks passed with zero failures: natural filename order, case and Unicode, missing files/directories, first/last boundaries, and exclusion of subfolders, parent folders and sibling folders. Four symbolic-link checks were skipped because Windows did not allow test-link creation. The code rejects reparse points, but these four runtime scenarios remain unverified on this account.
- 40 uninstall checks passed against the actual 0.3.3 EXE, with exit code zero and empty stderr, using isolated temporary files and dedicated test registry keys. No real installation or default-player registration was changed in this candidate pass.
- The production candidate was launched with a video path and inspected through Windows UI automation. Actual video clicks resumed playback, Space played/paused with Full screen focused without leaving fullscreen, wheel-up changed 80% to 85%, and Right/Left selected clip 10/clip 2. A saved screenshot shows the dark controls, light Loop lettering, visible checked state and paused frame. No light-mode option exists.
- An independent source review found no actionable issues in the changed controls, folder navigation or candidate version/build wiring.
- A fresh 23-file source staging folder containing spaces built successfully through the actual `Build Lost Eye.cmd`, with exit code zero and empty stderr. All eight build-input hashes matched the candidate record; the resulting EXE reported 0.3.3 and matched its own record. The three helper scripts and both embedded uninstall resources matched source. This rebuilt EXE was not separately launched or installed. Results are in `artifacts/Source Build Check 0.3.3 49d4a140dcea4035b141a01ceaf3be5b/verification-results.txt`.
- All eight candidate build inputs and the production EXE matched the original build record. PowerShell parsing, manifest XML parsing and `git diff --check` passed.

Local evidence is retained in `artifacts/candidate-controls-native-tests.txt`, `artifacts/candidate-folder-tests.txt`, `artifacts/candidate-controls-preview.png`, `artifacts/Candidate Uninstall Check 0.3.3 5f02c16e549843019ae03e152ca8b51f/`, and `artifacts/Lost-Eye-0.3.3-candidate-win-x64-build/`. These generated artifacts are not source files and are not committed.

GPU-device creation and rendered pixels were verified; this does not prove hardware decoding for every format. Audible A/V quality and personal video collections still need owner acceptance. Other machines, codecs, Windows N, HDR/protected content, and unsigned-download prompts are not certified. The real installed-version/default-player round trip recorded below belongs to 0.3.2 and was not repeated against the user's installed player for this candidate.

---

# Historical native Lost Eye 0.3.2 verification

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

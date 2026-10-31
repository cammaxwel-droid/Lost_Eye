# Lost Eye 0.3.3 release verification - 1 November 2026

This final build promotes the reviewed R3 player with unchanged playback/control behavior. Its visible title is Lost Eye 0.3.3, and its window class and single-instance mutex distinguish it from the earlier 0.3.3 candidate. Licence terms, notices, duck artwork and installation overwrite protection are unchanged. The candidate ZIP remains a separate local review artifact.

Fresh checks on this Windows laptop:

- 89 playback/control checks passed, exit 0, with zero failures and 481 presented frames on Intel UHD Graphics 620. MP4, MKV and WebM fixtures rendered, including deterministic first-frame/CANPLAY startup ordering and recovery from damaged files.
- 18 folder-navigation checks passed. Four symbolic-link scenarios were skipped because this account could not create those test links.
- 40 isolated uninstaller checks passed against the final executable. These did not change the real installation or default-player associations.
- MSVC 19.51.36260.0 and Windows SDK 10.0.28000.0 built the player and tests without warnings. Build-input and executable hashes are recorded in BUILD-INFO.json. Imported DLLs are Windows system components only.

Final executable SHA-256: 12066f10926382c1cf0c8509999408bfc4b83624d9f8dc8a6a73d14287912a67.

The portable download is Lost-Eye-0.3.3-win-x64.zip. It contains the executable, optional install/uninstall helpers, required notices, build instructions and matching native source. Lost-Eye-0.3.3-native-source.zip provides the same source separately. SHA-256 sidecars accompany both downloads. Development tools are needed only for source builds.

The runtime test reports remain local under artifacts/release-*-tests.txt; raw diagnostic reports containing machine-specific paths are excluded from published assets. The published release is pinned to commit 1701f1e701325eddfdd307e0e32abe8978a554a2. Later repository cleanup and security automation do not change its executable or assets.

Compatibility is not certified for every PC, codec, driver, Windows edition, HDR/protected video or audio configuration. The installed Start/Open with round trip was not repeated for this final build. Prior installation and candidate evidence remains in the historical report linked below, with its original scope preserved.


Earlier candidate checks, installation results and prior-version reports are preserved in the [complete release-time verification record](https://github.com/cammaxwel-droid/Lost_Eye/blob/1701f1e701325eddfdd307e0e32abe8978a554a2/VERIFICATION.md). The active runtime, icon and build inputs are unchanged by the repository cleanup.

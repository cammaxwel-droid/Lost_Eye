# Verification — 2026-10-28

Built on Windows x64 using the installed .NET 10 SDK. Release app and integration harness compile with zero warnings and zero errors.

31 integration checks passed. They exercised actual LibVLC decoding and the actual PlayerWindow implementation:

- MP4 (H.264/AAC), MKV (H.264/AAC) and WebM (VP9/Opus): decoded frames, advancing media time, and created video output.
- Pause holds position, seeking/resuming, volume and mute/unmute.
- Fullscreen state and restored bounds; resize to minimum size.
- Two complete loop restarts, loop-off stopping, staying stopped, and replay.
- Missing, empty, and damaged files; recovery by opening a valid file.
- Rapid file replacement; drop handler opening a file path containing spaces.
- Optional Open with registration and removal tested under a unique isolated HKCU test key. Existing synthetic defaults remained intact. Real file associations were not changed.
- Elapsed/total time formatting, including hour-long values.

The last run measured approximately 0.7–0.9 seconds to decoded frames plus advancing media time for the small local fixtures; this is not cold process startup or a performance guarantee. Integration-process working set was about 147 MiB after repeated opens. VLC reported Intel UHD Graphics 620 D3D11 hardware decoding.

Layout renders were inspected at this machine's 150% scaling at default and minimum sizes. A clipping issue was corrected. DrawToBitmap does not capture the GPU video surface or DWM window decoration, so those renders are layout evidence only.

Both native computer-use interfaces returned: `Computer Use native pipe is unavailable: failed to connect native pipe: The system cannot find the file specified. (os error 2)`.

Therefore the following still need a manual pass on the laptop: visible motion/quality and audible A/V sync; actual mouse clicks and seek dragging; Open File dialog interaction; actual Explorer drag/drop and Open with selection; fullscreen video continuity; multi-monitor/mixed-DPI transitions; long videos, 4K/HEVC/HDR, uncommon codecs and slow/removable storage. The test harness called the same drop handler but did not perform an Explorer drag gesture. No universal machine/codec compatibility is claimed.

VLC emitted taskbar thumbnail warnings (`SetThumbNailClip failed: 0x800706F4`) in the test environment, plus transient timestamp messages during some seek/restart operations. Decode/time/output tests still passed. These logs do not substitute for visible playback review.

Raw results: `artifacts/verification/results.json`. Layout renders: `artifacts/verification/layout-empty.png` and `layout-minimum.png`. Test assets are generated locally, separate from the portable distribution. The test harness exits and closes the player cleanly.

Packaging correction: the first candidate exposed slow cold plugin discovery. The final r2 package includes a plugin cache generated with its exact native binaries, and runtime discovery uses that cache. Windows initialization explicitly enables PerMonitorV2 DPI handling. The same 31 integration checks are repeated on the cached path before distribution. Early non-r2 artifacts are diagnostic candidates, not the final deliverable.

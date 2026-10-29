# Native Lost Eye 0.3.0 verification

Verified locally on Windows, 29 October 2026. This record applies to the native 0.3.0 build, not the retained .NET/LibVLC builds.

- C++ x64 release build succeeded. Production EXE is 396,800 bytes. Imported dependencies are Windows system DLLs only; no VLC, .NET, external player app, or separate VC runtime DLL is required.
- 38 automated native playback checks passed, with zero failures. Hardware D3D11 device and Media Foundation DXGI manager initialized on Intel UHD Graphics 620. Actual decoded colored pixels were read back from the graphics texture; 372 frames were presented during this test run.
- H.264/AAC MP4 and MKV, and VP9/Opus WebM fixtures rendered. Verified Back to start, Rewind 10s, Forward 10s, Skip to end, play after Skip, pause, seek, volume/mute API state, fullscreen/restore, resize, looping twice, disabling loop, staying stopped at the end, replay and three same-file reopens.
- The five transport buttons remained visible and non-overlapping at the minimum window width. They remain disabled for invalid or still-loading media.
- Missing, empty and damaged input paths were handled, and playback recovered with a valid file.
- Window responsiveness was checked with a deliberate two-second engine initialization delay. A separate close-during-initialization check passed.
- The installed production window rendered the MP4 fixture and visibly showed Back to start, Rewind 10s, Replay, Forward 10s and Skip to end in a dedicated transport row.
- Installed EXE SHA-256 matches the production artifact. Start menu shortcut targets the installed version. All 12 supported extension defaults and UserChoice values were identical before and after registration.
- Windows SHAssocEnumHandlers enumerated Lost Eye with its installed path and friendly name. IAssocHandler::Invoke succeeded; the existing production window changed to the requested sample.mkv filename. Command-line forwarding of a path containing spaces also succeeded.

Boundaries: the graphics device/frame path was verified; hardware decoding for every codec was not. Audible sound quality, subjective smoothness, mouse-driven drag/drop and Open File interaction, and other machines, drivers, HDR/protected media or Windows N were not independently verified. Please try your own videos and confirm sound and controls on the laptop. No claim of universal codec compatibility is made.

Detailed local reports are retained in the ignored artifacts/Lost-Eye-0.3.0-win-x64-tests and artifacts/native-0.3.0 folders. Existing legacy source and older local packages were not overwritten.

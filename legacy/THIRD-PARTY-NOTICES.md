# Third-party notices

Lost Eye dynamically uses the following components. No third-party binaries were modified. Keep these notices, license texts, and source access with redistributed copies.

| Component | Version | License / provenance |
|---|---|---|
| LibVLCSharp | 3.10.2, commit 3082c09ca955131c705fc4767fde03e9c4a14999 | LGPL-2.1-or-later; Copyright VideoLAN and contributors |
| VideoLAN.LibVLC.Windows | 3.0.24, official NuGet package | Core LGPL-2.1-or-later; plugin set contains other licenses including GPL-2.0-or-later |
| .NET runtime / Windows Desktop | 10.0.14 | MIT and component-specific notices; Copyright .NET Foundation and contributors |

The native NuGet manifest labels the package LGPL-2.1-or-later. Inspection found `plugins/codec/libx26410b_plugin.dll`; VLC 3.0.24 `modules/codec/x264.c` explicitly declares GPL-2.0-or-later. Accordingly this distribution and Lost Eye's source use GPL-2.0-or-later, retaining LGPL notices for the engine and managed wrapper. This avoids representing the entire bundled plugin set as LGPL-only.

Full license texts and .NET third-party notices are in `licenses/`. VideoLAN author attribution is in `licenses/VLC-AUTHORS.txt`; original copyright headers and component licenses are retained in the included upstream source archives. The VLC source includes contrib build recipes, URLs, hashes, and patches for codec and support dependencies (including FFmpeg, x264, dav1d, libvpx, aom, libogg, libvorbis, FLAC, Opus, Speex, libass, FreeType, HarfBuzz, FriBidi, libpng, zlib, and others). See SOURCES.md for access to those dependency sources.

The runtime loads `LibVLCSharp.dll` and `libvlc/win-x64/` dynamically. You may replace these with compatible modified versions, and debug/reverse-engineer the application as necessary to exercise your LGPL rights. Do not mix incompatible architectures or library/plugin versions. There is no anti-tamper restriction.

VideoLAN guidance: https://docs.videolan.me/vlc-user/en/support/faq/legalconcerns.html
.NET licenses: https://github.com/dotnet/runtime and https://github.com/dotnet/windowsdesktop

Software licensing is separate from any patent/codec obligations in the jurisdiction and use case where a binary is distributed. No claim of universal codec, DRM, hardware, or patent clearance is made.

The original duck was generated for Lost Eye; it is not the VLC icon, a stock character, or an endorsement by any other project. The supplied source assets may be used with this project.

# Native Lost Eye 0.3.0 notices

Lost Eye source code: Copyright (c) 2026 Lost Eye contributors. GPL-2.0-or-later; see LICENSE and licenses/GPL-2.0.txt. The native source is included in Lost-Eye-native-source.zip.

The program uses Microsoft Windows APIs: Media Foundation, Direct3D 11, DXGI, Shell, Win32 controls, and COM. These are operating-system components and are not redistributed in this package. No VLC, LibVLCSharp, FFmpeg, .NET runtime, Windows Media Player app, or external codec library is included or launched. The Microsoft C++ runtime is statically linked by the licensed Microsoft Build Tools; no separate Visual C++ redistributable installation is required.

Video and audio support depend on Windows codecs, the graphics driver, and the media file. Hardware acceleration is requested through the Direct3D device and Media Foundation DXGI device manager. Windows may use software decoding for formats the GPU cannot decode. No universal codec or GPU guarantee is made.

The original yellow duck artwork was generated for this project. It is retained unchanged and is not a third-party player logo. Asset provenance and the generation prompt are included in the source archive.

References:
https://learn.microsoft.com/en-us/windows/win32/medfound/mf-media-engine-dxgi-manager
https://learn.microsoft.com/en-us/windows/win32/api/mfmediaengine/nf-mfmediaengine-imfmediaengine-onvideostreamtick
https://learn.microsoft.com/en-us/windows/win32/api/mfmediaengine/nf-mfmediaengine-imfmediaengine-setloop
https://learn.microsoft.com/en-us/windows/win32/shell/app-registration

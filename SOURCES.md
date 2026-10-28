# Corresponding sources and build references

The portable ZIP includes `sources/Lost-Eye-source.zip` (the app, assets, tests, scripts, dependency lock, and notices), `sources/vlc-3.0.24.tar.xz`, and `sources/libvlcsharp-3.10.2-source.zip`.

Official originals:

- VLC 3.0.24: https://download.videolan.org/pub/videolan/vlc/3.0.24/vlc-3.0.24.tar.xz
  SHA-256: E7CAB503D1D7D5849B89D2CF0E1EE60D0EF6D012407791B644B9CFC0CC225FDF
- LibVLCSharp exact NuGet source commit: https://github.com/videolan/libvlcsharp/tree/3082c09ca955131c705fc4767fde03e9c4a14999
  Included archive SHA-256: 7C7B56D71721E7F3F12607E629AECA9F8416119A24CD4D016CF7F8F1F9940336
- Native NuGet package: https://www.nuget.org/packages/VideoLAN.LibVLC.Windows/3.0.24
- Native packaging/build scripts: https://code.videolan.org/videolan/libvlc-nuget
- VideoLAN contrib source archive mirror: https://download.videolan.org/pub/contrib/
- VLC build instructions and dependency source download/patch recipes: `vlc-3.0.24/INSTALL`, `extras/package/win32/`, and `contrib/src/` inside the included VLC source archive. The recipes name the component revisions, upstream source URLs, patches and checksums; these are part of the source distribution, not custom codecs.
- .NET runtime: https://github.com/dotnet/runtime/tree/v10.0.14
- Windows Forms: https://github.com/dotnet/winforms/tree/v10.0.14
- WPF components in the self-contained desktop runtime: https://github.com/dotnet/wpf/tree/v10.0.14
- Desktop runtime packaging: https://github.com/dotnet/windowsdesktop/tree/v10.0.14

When distributing this build from another download site, preserve the source archives and provide equivalent access to the upstream dependency sources and build scripts alongside the binary download. If modifying native components, provide the actual corresponding modified sources, dependency revisions, and build configuration, not merely these links. This package does not contain every upstream dependency's source tarball; the included contrib recipes and official mirror provide the upstream retrieval path.

Rebuild Lost Eye with the .NET 10 SDK and `scripts/Build-Portable.ps1`. The managed package lock records exact NuGet package content hashes. The `SHA256SUMS.txt` in the portable folder inventories shipped files. Keep the complete portable folder together; replacing only the executable is not a standalone installation.

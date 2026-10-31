# Native Lost Eye third-party notices

Applies to native Lost Eye 0.3.3. The project owner's portions use the personal source terms in [LICENSE](LICENSE). Third-party software retains its own terms; the project license does not relicense Microsoft components.

| Component | How it is used | Distribution and terms |
|---|---|---|
| Windows Media Foundation, Direct3D 11, DXGI, Win32, COM and Shell APIs | System media, graphics, windowing and file associations | Supplied by Windows; these DLLs are not included in this package. Windows licensing and component/codec availability apply. |
| Microsoft C/C++ runtime | Release runtime object code statically linked with /MT | Included inside Lost Eye.exe; copyright Microsoft Corporation and applicable contributors. Subject to applicable Microsoft development and redistribution terms. No separate Visual C++ runtime installation is required. |
| Original yellow duck artwork | Embedded icon | Generated for Lost Eye; preserved assets and generation record are in the source archive. No third-party player logo was used. This is a provenance statement, not a guarantee of uniqueness or exclusive copyright. |

This inventory covers the current native package. Windows supplies the media services; the build does not download packages or bundle a separate codec library. Earlier implementations retain their own notices and terms as described in [LICENSING.md](LICENSING.md).

The Microsoft runtime is built using the local installed toolchain. Its compiler/SDK versions and build input hashes are recorded in BUILD-INFO.json; toolchain components themselves are not included in the source archive. Builders and distributors must satisfy the applicable Microsoft terms. The license file for Lost Eye does not grant rights to redistribute Microsoft's tools, headers or runtime source.

Windows may decode in software when the GPU or driver cannot decode a format. Format support depends on Windows components, graphics drivers and the media file. No universal codec, GPU, HDR, DRM or patent clearance is claimed. Optional Windows codec extensions may have separate terms.

References:

- [Microsoft redistribution information](https://learn.microsoft.com/en-us/visualstudio/releases/2026/redistribution)
- [Visual Studio Community terms](https://visualstudio.microsoft.com/license-terms/vs2026-ga-community/)
- [Visual Studio Build Tools terms](https://visualstudio.microsoft.com/license-terms/vs2026-ga-diagnostic-buildtools/)
- [Windows Media Foundation DXGI manager](https://learn.microsoft.com/en-us/windows/win32/medfound/mf-media-engine-dxgi-manager)
- [Windows application registration](https://learn.microsoft.com/en-us/windows/win32/shell/app-registration)

See [LICENSING.md](LICENSING.md) for previous GPL grants, ownership limits and commercial review considerations.

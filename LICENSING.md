# Lost Eye licensing

Starting with native **0.3.1**, the project owner's original native code, native build scripts, current documentation and artwork are offered under the [Personal Source License](LICENSE). It permits private personal use, source builds and modifications. Commercial use and redistribution require separate written permission from the owner. This is source-available software, not an OSI open-source release.

## Earlier releases remain licensed as supplied

The earlier .NET/LibVLC versions **and native 0.3.0** were supplied under GPL-2.0-or-later. The artwork was also included with those releases. Changing the license for a later copy does not withdraw existing GPL permissions, including commercial redistribution permitted by GPL. Anyone with a valid earlier GPL copy retains those rights; this new file does not make that code or artwork retrospectively exclusive.

The repository preserves the original notice in `licenses/LEGACY-GPL-NOTICE.txt`, the full GPL text in `licenses/GPL-2.0.txt`, and the earlier notices in `legacy/`. Legacy implementation files in `src/`, `tests/`, `tools/` and their build scripts retain their previous terms. These legacy materials are not included in the new native portable package.

The owner can offer a separate commercial license only for rights they hold. A contributor's work or third-party code cannot be relicensed merely by editing this file. Repository commit history and the artwork generation record are provenance evidence, not an independent legal chain-of-title audit. No claim is made that generated artwork has exclusive copyright protection in every jurisdiction.

## What the native application depends on

- Windows Media Foundation, Direct3D 11, DXGI and other Windows APIs provide the media and windowing services. They are supplied by Windows, not bundled as Lost Eye source.
- Microsoft C/C++ runtime object code is linked into the EXE with `/MT`. It remains Microsoft software under the applicable development and redistribution terms. A separate runtime installer is unnecessary, but that does not mean the EXE contains only project-owned code.
- The native build does not use or ship VLC, libVLC, LibVLCSharp, FFmpeg, .NET or another player application. Legacy VLC notices still matter for legacy packages.
- Using Windows codecs does not establish universal patent clearance, codec compatibility or rights to protected media.

See [THIRD-PARTY-NOTICES.md](THIRD-PARTY-NOTICES.md) for the native inventory and [BUILDING.md](BUILDING.md) for build prerequisites. Building the application from its supplied source still requires Windows and a properly licensed Microsoft toolchain; it is not a rebuild of Windows or Microsoft's runtime from project-owned source.

## Commercial review references

VideoLAN does not automatically charge a royalty merely because an application is sold. Its software licenses and any codec patent obligations are separate. Removing the VLC dependency removes it from the native package's dependency inventory; it is not a promise that all legal obligations disappear.

Before a sale or transfer, review ownership, previous grants, contributor permissions, runtime redistribution and relevant codec/patent rights with qualified legal counsel. This repository has not undergone that legal review and does not promise that no fees or obligations can ever arise.

Official references checked 29 October 2026:

- [GNU: release under both GPL and another license](https://www.gnu.org/licenses/gpl-faq.en.html#ReleaseUnderGPLAndNF), [contributor permissions](https://www.gnu.org/licenses/gpl-faq.en.html#Consider), and [existing public rights](https://www.gnu.org/licenses/gpl-faq.en.html#CanDeveloperThirdParty).
- [VideoLAN legal FAQ](https://docs.videolan.me/vlc-user/desktop/3.0/en/support/faq/legalconcerns.html).
- [Visual Studio Community 2026 terms](https://visualstudio.microsoft.com/license-terms/vs2026-ga-community/), [Build Tools terms](https://visualstudio.microsoft.com/license-terms/vs2026-ga-diagnostic-buildtools/), and [redistribution information](https://learn.microsoft.com/en-us/visualstudio/releases/2026/redistribution). Individual Community licensing permits developing one's own applications for sale; eligibility and conditions still apply. Installing Build Tools alone does not prove a valid entitlement to use them for this project.
- [Windows 11 terms, including video patent notices](https://www.microsoft.com/content/dam/microsoft/usetm/documents/windows/11/oem-%28pre-installed%29/Useterms_OEM_Windows_11_EnglishGreatBritain.pdf).
- [OpenAI terms: output rights and similarity](https://openai.com/policies/row-terms-of-use/). Asset generation provenance is in `assets/GENERATION.md` in the source archive.

# Security

## Supported version

Security fixes target the latest native release, currently 0.3.4. Older releases and historical implementations are retained for provenance and are not maintained. Download builds only from this repository's GitHub Releases and compare the supplied SHA-256 checksums.

## Report a vulnerability privately

Use [Report a vulnerability](https://github.com/cammaxwel-droid/Lost_Eye/security/advisories/new). Include the affected version, Windows version, a minimal reproduction and the expected impact. Do not post credentials, private videos or exploit details in a public issue. Avoid attaching malicious media to a public discussion; describe it first and coordinate a private transfer with the maintainer if needed.

## Project boundaries

Lost Eye opens local files and passes media decoding to Windows Media Foundation. The application uses Direct3D and Windows APIs, with a statically linked Microsoft runtime. It has no account, server or application auto-updater. Keep Windows and graphics drivers updated. Codec support and security fixes in those system components are controlled by their suppliers.

Installation and removal operate within the current Windows account, preserve videos and default-player choices, and refuse conflicting or unowned files. Security review should cover local-file/path handling, media-event and window-message boundaries, installer/uninstaller ownership checks, registry changes, build scripts and release integrity.

## Ongoing checks

The security workflow runs CodeQL for C++ and GitHub Actions on pull requests, pushes to main, and weekly. The Windows analysis builds the native application using the repository build script. Actions are pinned to commit hashes; Dependabot checks those action references weekly. These checks do not publish builds or merge pull requests.

GitHub secret scanning, push protection and private vulnerability reporting supplement the source checks. Findings require triage; an automated scan or clean test run is not a guarantee that all vulnerabilities have been found. See [VERIFICATION.md](VERIFICATION.md) for measured runtime coverage and limitations.

# Release signing setup

Native 0.3.4 release binaries are signed through [SignPath.io](https://about.signpath.io) with a free certificate from [SignPath Foundation](https://signpath.org). The certificate is issued to SignPath Foundation, not to this project; see the code signing policy in [README.md](README.md#code-signing-policy) for the required attribution, roles and privacy statement.

This file records what must be configured in SignPath and GitHub before the workflows can produce signed binaries. No certificate, token or other secret is stored in this repository.

## One-time SignPath setup

1. Apply at <https://signpath.org/apply> for the `cammaxwel-droid/Lost_Eye` project. Eligibility depends on the GPL-2.0-or-later license, the public repository and verifiable build origin described in the README. SignPath reviews reputation and can decline.
2. Enable multi-factor authentication on the GitHub account and the SignPath account.
3. Add the predefined Trusted Build System **GitHub.com** to the SignPath organization, link it to the project, and install the [SignPath GitHub App](https://github.com/apps/signpath) with access to this repository.
4. Create an artifact configuration for the EXE with file metadata restrictions: product name `Lost Eye`, product version `0.3.4`, enforced. Those values already exist in `native/LostEye.rc`.
5. Create signing policies (a test policy and a release policy). Each release signing request requires manual approval by the repository owner. Approval can take days, so the workflow must not wait for it.
6. Record the organization ID, project slug, signing-policy slug and artifact-configuration slug in the repository variables `SIGNPATH_ORGANIZATION_ID`, `SIGNPATH_PROJECT_SLUG`, `SIGNPATH_SIGNING_POLICY_SLUG` and `SIGNPATH_ARTIFACT_CONFIGURATION_SLUG`. Store the submitter API token in the repository secret `SIGNPATH_API_TOKEN`. Never commit the token.

## Two-phase workflow

1. `.github/workflows/sign-release.yml` builds on a GitHub-hosted Windows runner with `scripts/Build-Native.ps1`, uploads the unsigned EXE as a workflow artifact, and submits a SignPath request with `wait-for-completion: false`. It records the signing request ID and approval URL in the run summary. It runs on `workflow_dispatch` and on published releases. It does not download or verify a signed binary, because approval may take days.
2. After SignPath approves, run `.github/workflows/download-signed.yml` manually with the request ID from step 1. It checks the SignPath connector status, downloads the signed artifact through the same GitHub Actions connector endpoint used by the official SignPath action, verifies that `Get-AuthenticodeSignature` reports `Valid`, and uploads the signed EXE as a workflow artifact. An optional `tag` input attaches that EXE to an existing GitHub release with `gh release upload`.
3. Verify the hosted runner's Visual Studio licence covers this build before relying on it. The build script requires MSVC x64/x86 tools and a Windows SDK via `vswhere.exe`.

The published v0.3.4 ZIP remains unsigned. A signed EXE must be packaged into a new ZIP before it can replace that download.

## Local confirmation after a signed release

1. Download the signed EXE and confirm `Get-AuthenticodeSignature` shows `Valid`, a chain to a trusted root and a valid timestamp.
2. Back up the installed unsigned copy at `%LOCALAPPDATA%\Programs\Lost Eye\0.3.4\Lost Eye.exe` before replacing it.
3. Launch the signed EXE and check `Microsoft-Windows-CodeIntegrity/Operational` for new events 3118, 3033 or 3077. No new events means Smart App Control is no longer blocking it.
4. Repackage the portable ZIP with the signed EXE so downloads inherit the signature.

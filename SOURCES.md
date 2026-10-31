# Native source and build inputs

The native portable package includes Lost-Eye-native-source.zip: application code, resource/manifest files, original icon assets, build and packaging scripts, the double-click build launcher, and current documentation/license. Extract it before building; follow [BUILDING.md](BUILDING.md).

The production entry point is native/LostEye.cpp. No prebuilt Lost Eye executable or cached build output is required to compile it. Build dependencies are Windows, an appropriately licensed Microsoft C++ toolchain, and the Windows SDK. The native build does not fetch packages, codec implementations or external application binaries.

BUILD-INFO.json records the toolchain, source/build input hashes and generated EXE hash. SHA256SUMS.txt inventories the portable package. These records identify inputs and outputs; compiler metadata and timestamps mean builds are not claimed to be byte-for-byte reproducible.

The source archive contains project source, not source for Windows or Microsoft's compiler/runtime. Those components retain Microsoft's terms. Personal source builds are allowed by LICENSE. See LICENSING.md before commercial use or redistribution.

Earlier implementations, component download hashes, rebuild instructions and original license notices remain together in the [historical source snapshot](https://github.com/cammaxwel-droid/Lost_Eye/tree/1701f1e701325eddfdd307e0e32abe8978a554a2/legacy). They were removed from the current branch because the native build does not depend on them. Current native packages use an explicit file list. Existing tags and Git history preserve the older materials and their terms.

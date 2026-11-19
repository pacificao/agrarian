# Desktop wallet release checklist

Start with Windows x86-64 and Ubuntu 24.04 x86-64. List Ubuntu explicitly as
the supported Linux target until other distributions are tested.

1. Identify the reviewed source commit and verify compatibility with the live
   network, including consensus rules, sync and wallet database handling. Do
   not infer compatibility from branch names or reuse unlabeled binaries.
2. Build both packages from the same reviewed commit in separate clean,
   non-production environments using the [wallet build guide](build-wallets.md).
   Record the full commit, build commands, toolchain and dependencies. Keep
   generated objects and packages out of Git.
3. Test each packaged artifact on a clean OS of the advertised version. Verify
   GUI launch, version, dependencies, network sync, and disposable regtest
   wallet creation, send/receive, backup/restore and restart. Check the Windows
   package on Windows; cross-compilation alone doesn't validate it.
4. Use a verified signing key for the release tag and SHA256 checksum manifest.
   Record the signing fingerprint in the release documentation. Authenticode-sign
   the Windows package if a trusted code-signing credential is available and
   accurately label its signing status.
5. Upload the tested packages, signed checksum manifest, compatibility notes,
   and verification instructions to a draft GitHub Release. Verify downloads and
   signatures from a second machine before making it public. Link the website to
   the published release only after those checks pass.

No desktop wallet packages are published at the time this checklist was added.
The web wallet and desktop wallet are separate products; these build steps do
not change the hosted web wallet or any network node.

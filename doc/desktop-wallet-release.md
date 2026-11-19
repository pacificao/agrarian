# Desktop wallet release checklist

Start with Windows x86-64 and Ubuntu 24.04 x86-64. List Ubuntu explicitly as
the supported Linux target until other distributions are tested.

1. Identify the reviewed source commit and pass the
   [hybrid release gates](modernization.md#hybrid-release-gates), including an
   offline mainnet replay. Verify wallet database handling as well. Do
   not infer compatibility from branch names or reuse unlabeled binaries.
2. Build both packages from the same reviewed commit in separate clean,
   non-production environments using the [wallet build guide](build-wallets.md).
   Record the full commit, build commands, toolchain and dependencies. Keep
   generated objects and packages out of Git.
3. Test each packaged artifact on a clean OS of the advertised version. Verify
   GUI launch, version, dependencies, network sync, and disposable regtest
   wallet creation, send/receive, encryption, backup/restore and restart.
   Include an existing legacy `wallet.dat` fixture without using live keys.
   Check the Windows installer on Windows and the Linux download on a clean
   Ubuntu install without development packages. Cross-compilation alone
   doesn't validate a package.
4. Use a verified signing key for the release tag and SHA256 checksum manifest.
   Record the signing fingerprint in the release documentation. Authenticode-sign
   the Windows package if a trusted code-signing credential is available and
   accurately label its signing status.
5. Upload the tested packages, signed checksum manifest, compatibility notes,
   and verification instructions to a draft GitHub Release. Verify downloads and
   signatures from a second machine before making it public. On the website,
   provide Windows and Linux download buttons with version, supported OS and a
   verification link. Point them to versioned GitHub release assets, and keep the
   web wallet and source-build launcher available as separate options.

Candidate automation
--------------------

The **Desktop wallet candidate** workflow builds the assigned commit on
Ubuntu 24.04 x86-64. The source export omits old checked-in objects, compiled
dependencies, download stamps and generated build state; `depends` fetches and
verifies the sources from its pinned recipes. Outputs are temporary CI
artifacts, not GitHub Release downloads. Both package jobs verify the artifact
checksum and exercise the installed GUI and a disposable regtest wallet.

The Ubuntu smoke job runs in a fresh 24.04 userspace with runtime packages
only; the Windows job uses a fresh Windows Server x64 runner and the NSIS
installer. Still test interactively on the actual advertised desktop OSes
(initially Ubuntu 24.04 Desktop x64 and Windows 11 x64), including starting
from shortcuts, syncing from an empty data directory and opening a disposable
`wallet.dat` made by the previous released version. CI tests alone do not
establish these results. Never upload a live wallet to CI.

Both platforms must pass on the same commit and hashes. Obtain an approved
release-signing key and publish its fingerprint through a trusted channel;
the 2.0.0 source tag is not a signed baseline. Do not create a public tag,
update the website, or publish candidate binaries while a gate is open.
The hosted web wallet is separate from the native packages.

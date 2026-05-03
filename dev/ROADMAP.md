# Agrarian 2.0 Development Roadmap

This file tracks the 2.0 line before it becomes the main Agrarian codebase.

## Current Release Goal

Agrarian 2.0 should be the modern, buildable baseline for the coin:

- Ubuntu daemon builds
- Ubuntu Qt wallet builds
- Windows daemon cross-builds
- Windows Qt wallet cross-builds
- ARM64 daemon builds for server nodes
- clean enough docs and scripts for repeatable fresh-server builds
- stable network behavior with existing Agrarian nodes

## Validation Before Promoting 2.0 To Main

- Linux daemon: validated on Ubuntu with `contrib/build-linux.sh`.
- Linux Qt wallet: validated on Ubuntu with `contrib/build-linux-wallet.sh`.
- Daemon smoke test: validated with `contrib/smoke-test-daemon.sh`.
- Wallet smoke test: validated with `contrib/smoke-test-wallet.sh`.
- Qt smoke test: validated with `contrib/smoke-test-qt.sh`.
- Windows daemon: validated with the build menu on x86_64 Ubuntu.
- Windows Qt wallet: validated with the build menu on x86_64 Ubuntu.
- Linux ARM64 daemon: validated with the build menu cross-build from x86_64 Ubuntu.
- Linux ARM64 Qt wallet: native ARM64 build only for now; x86_64 menu guard validated.

## Build Script Targets

The build menu is the preferred entry point:

```bash
./contrib/agrarian-build-menu.sh
```

Supported targets:

- Linux daemon and CLI tools
- Linux Qt GUI wallet
- Windows daemon and CLI tools
- Windows Qt GUI wallet
- Linux ARM64 daemon and CLI tools
- Linux ARM64 Qt GUI wallet, native ARM64 host only

The script should:

- clone or fast-forward the selected branch
- install Ubuntu packages needed for the selected target
- refuse unsafe root execution unless explicitly overridden for controlled CI/root environments
- stop a running user daemon gracefully before replacing binaries
- refuse to force-close the GUI wallet
- restore a daemon it stopped when the build completes

## Dependency Policy

Already modernized in the 2.0 line:

- OpenSSL LTS path
- Boost current modern path
- Qt 6.8 LTS path
- expat
- freetype
- zlib
- libevent
- zeromq
- qrencode
- miniupnpc
- GMP
- protobuf

Berkeley DB stays on 4.8.30 for the 2.0 release to preserve wallet compatibility.
Do not jump to BDB 18.x in 2.0. A future wallet database migration should be a
planned project, likely toward SQLite, with explicit wallet backup and migration
tooling.

## Known Follow-Up Work

- Re-run Windows daemon and Windows Qt wallet validation after every dependency change.
- Keep ARM64 Qt wallet as native-only until a reliable cross-Qt path is designed.
- Reduce remaining compiler warning noise where it affects correctness.
- Replace deprecated `std::random_shuffle` usage.
- Audit old Boost and LevelDB warning areas without changing consensus behavior casually.
- Build a formal release packaging process for Linux and Windows artifacts.
- Add repeatable clean-machine CI once the local build menu is stable.
- Plan the SQLite wallet migration as a post-2.0 milestone.
- Continue wallet UX modernization after the 2.0 build baseline is promoted.

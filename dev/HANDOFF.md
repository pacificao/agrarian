# Agrarian Codex Handoff

## Project State

Active repo:

```bash
/var/www/root_builds/agrarian
```

Active branch:

```bash
2.0
```

The 2.0 branch is intended to become the main development line after final build
validation. The worktree may contain build-generated files after local compiles;
do not commit generated build output.

## Current Build Baseline

Validated locally during the final pre-main pass:

```bash
env JOBS=8 ./contrib/build-linux.sh
env JOBS=8 ./contrib/build-linux-wallet.sh
./contrib/smoke-test-daemon.sh
./contrib/smoke-test-wallet.sh
./contrib/smoke-test-qt.sh
```

Expected successful artifacts:

```bash
src/agrariand
src/agrarian-cli
src/agrarian-tx
src/qt/agrarian-qt
```

Additional validation completed during this pass:

```bash
ALLOW_ROOT_BUILD_MENU=1 AGRARIAN_MENU_CHOICE=windows-daemon WORKDIR=/var/www/root_builds/agrarian BRANCH=2.0 JOBS=8 ./contrib/agrarian-build-menu.sh
ALLOW_ROOT_BUILD_MENU=1 AGRARIAN_MENU_CHOICE=windows-qt WORKDIR=/var/www/root_builds/agrarian BRANCH=2.0 JOBS=8 ./contrib/agrarian-build-menu.sh
ALLOW_ROOT_BUILD_MENU=1 AGRARIAN_MENU_CHOICE=linux-arm64-daemon WORKDIR=/var/www/root_builds/agrarian BRANCH=2.0 JOBS=8 ./contrib/agrarian-build-menu.sh
ALLOW_ROOT_BUILD_MENU=1 AGRARIAN_MENU_CHOICE=linux-arm64-qt WORKDIR=/var/www/root_builds/agrarian BRANCH=2.0 JOBS=8 ./contrib/agrarian-build-menu.sh
```

The ARM64 daemon cross-build produced ARM aarch64 ELF binaries. The ARM64 Qt
target correctly refused to run on x86_64 and remains native-only.

## Build Menu

Use the menu for fresh-machine testing:

```bash
curl -L https://raw.githubusercontent.com/pacificao/agrarian/2.0/contrib/agrarian-build-menu.sh -o agrarian-build-menu.sh
chmod +x agrarian-build-menu.sh
BRANCH=2.0 ./agrarian-build-menu.sh
```

For controlled root-only environments only:

```bash
ALLOW_ROOT_BUILD_MENU=1 BRANCH=2.0 ./contrib/agrarian-build-menu.sh
```

The menu now includes Linux, Windows, and ARM64 targets. ARM64 daemon cross-build
from x86_64 is supported. ARM64 Qt is native-only for now.

## Network Notes

Mainnet P2P port:

```text
51336
```

Seed nodes currently use:

```text
node1.agrariancoin.com
node2.agrariancoin.com
node3.agrariancoin.com
node4.agrariancoin.com
node5.agrariancoin.com
```

The daemon has been used to test peer visibility with another node at
`dev.barnealogy.com`. Recheck connectivity after major consensus or network
changes.

## Important Decisions

- Keep Berkeley DB at 4.8.30 for 2.0 wallet compatibility.
- Do not upgrade to BDB 18.x in this release.
- Treat a SQLite wallet backend as a future migration project with backup,
  migration, and rollback documentation.
- For now, Linux validation is the fastest correctness loop, but Windows builds
  must pass before release.
- Do not assume Linux success guarantees Windows success; cross-build and Qt
  plugin/link behavior can fail independently.

## Known Warning Areas

- `std::random_shuffle` deprecation warnings remain.
- Some vendored dependency warnings remain.
- Some old Boost/LevelDB warning noise remains.
- Cleanup should be correctness-driven, not broad cosmetic churn before release.

## Safe Cleanup Pattern

Before committing, stage intentional source/doc/script files first, then revert
unstaged build output:

```bash
git add <intentional files>
git restore .
git clean -nd
```

Only run `git clean -fd` after reviewing the dry-run output.

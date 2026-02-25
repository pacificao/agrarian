Installer (Ubuntu)
==================

Overview
---------------------
The Agrarian installer is `installer/agrarian-installer.sh`. It automates the Ubuntu build steps by:

- Building the deterministic `depends/` prefix (`make -C depends HOST=... USE_WALLET=... -j...`).
- Configuring the project with the depends `config.site` and prefix.
- Building the daemon and CLI utilities (via `--action daemon` or `--action all`).
- Building the Qt wallet on Linux hosts (via `--action qt`, or `--action all` when Qt is enabled).
- Cross-compiling Qt wallets for Windows and Linux ARM targets (via `--action qt --qt-target ...`).

It does not:

- Install system packages (it only checks for required toolchains and prints an `apt-get` command if missing).
- Run `make install` or copy binaries into `/usr/local`.
- Package platform installers (e.g., Windows/macOS/ARM Qt wallet bundles).
- Run or configure the daemon after the build.

Quick Start (Ubuntu/Debian)
---------------------
Clone the repo and enter it (standard `git clone` workflow), then:

Show installer help:

```bash
./installer/agrarian-installer.sh --help
```

Build the daemon (runs depends, configure, then `make -C src agrariand`):

```bash
./installer/agrarian-installer.sh --action daemon
```

Build the Qt wallet on Linux (runs depends Qt target when `HOST` is Linux, then `make -C src/qt agrarian-qt`):

```bash
./installer/agrarian-installer.sh --action qt
```

Build Qt wallets for cross targets (examples):

```bash
./installer/agrarian-installer.sh --action qt --qt-target win64
./installer/agrarian-installer.sh --action qt --qt-target win32
./installer/agrarian-installer.sh --action qt --qt-target armhf
./installer/agrarian-installer.sh --action qt --qt-target aarch64
./installer/agrarian-installer.sh --action qt --qt-target all
```

Expected outputs:

- Depends prefix: `depends/<host-triplet>/`
- Daemon: `src/agrariand`
- CLI utilities (when using `--action all` or a full top-level build): `src/agrarian-cli`, `src/agrarian-tx`
- Qt wallet (Linux): `src/qt/agrarian-qt`
- Qt wallet (Windows): `src/qt/agrarian-qt.exe`

Run the daemon:

```bash
./src/agrariand
```

Common Installer Options
---------------------
`--action <depends|daemon|qt|all>`

- `depends` builds only the `depends/` prefix.
- `daemon` builds the daemon (`src/agrariand`). If depends are missing, the installer builds them first.
- `qt` builds the Qt wallet (`src/qt/agrarian-qt`). If depends are missing, the installer builds them first.
- `all` builds depends and then runs top-level `make` in the repo root.

`--qt-target <native|win64|win32|armhf|aarch64|all>`

- `native` uses `--host` as-is (default).
- `win64` maps to `x86_64-w64-mingw32`.
- `win32` maps to `i686-w64-mingw32`.
- `armhf` maps to `arm-linux-gnueabihf`.
- `aarch64` maps to `aarch64-unknown-linux-gnu`.
- `all` builds native + all cross targets listed above.

`--host <triplet>`

- Sets the build host triplet (default: `x86_64-pc-linux-gnu`).
- Passed to `depends` as `HOST=<triplet>`.

`--wallet <0|1>`

- Controls wallet support.
- Passed to `depends` as `USE_WALLET=<0|1>`.
- When `--wallet 0`, the installer also adds `--disable-wallet` during `configure`.

`--dry-run`

- Prints the commands the installer would run, without executing them.

`NO_QT=1` in depends

- The installer does not expose `NO_QT`.
- If you need to skip Qt in depends, run it manually:

```bash
make -C depends HOST=x86_64-pc-linux-gnu NO_QT=1
```

Troubleshooting
---------------------
Boost library naming/layout (the `-mt` suffix)

- Depends builds may produce Boost libs with suffixes like `libboost_thread-gcc-mt-1_64.a`.
- The installer checks for `libboost_thread*.a` and `libboost_system*.a`, so it tolerates `-mt` and versioned names.
- If you are configuring manually, keep `--with-boost=<depends-prefix>` (the installer sets this for you) to avoid system Boost fallback.

`config.site` and PATH pitfalls

- The installer configures with `CONFIG_SITE=depends/<host>/share/config.site`.
- That `config.site` prepends `depends/<host>/native/bin` to `PATH`. Do not overwrite `PATH` with a minimal value; ensure `/usr/bin` and other system paths remain available.
- If `config.site` is missing, rebuild depends:

```bash
./installer/agrarian-installer.sh --action depends
```

Qt pkg-config files missing in the depends prefix

The installer requires these `.pc` files in `depends/<host>/(lib|share)/pkgconfig`:

- `Qt5Core.pc`
- `Qt5Gui.pc`
- `Qt5Network.pc`
- `Qt5Widgets.pc`

Fix by rebuilding depends with Qt enabled:

```bash
./installer/agrarian-installer.sh --action qt
```

Forcing rebuilds

- Use `--reset-depends` to delete `depends/work`, `depends/built`, and `depends/<host>`, then re-run the installer action you need.
- If you need a fresh depends build without deleting the prefix, re-run the depends build (same command the installer prints on failure):

```bash
make -C depends HOST=<host> USE_WALLET=<wallet> -j<JOBS>
```

Qt Cross-Compilation Packages (Ubuntu/Debian)
---------------------
Install the common Linux build tools (from `depends/README.md`):

```bash
sudo apt-get install make automake cmake curl g++-multilib libtool binutils-gold bsdmainutils pkg-config python3 patch
```

Then install toolchains per target:

- win64: `g++-mingw-w64-x86-64` (see `doc/build-windows.md`)
- win32: `g++-mingw-w64-i686` (see `doc/build-windows.md`)
- armhf: `g++-arm-linux-gnueabihf` and `binutils-arm-linux-gnueabihf`
- aarch64: `g++-aarch64-linux-gnu` and `binutils-aarch64-linux-gnu`

If any toolchain binaries are missing, the installer prints a single `apt-get install` command that includes the required packages for the selected Qt targets.

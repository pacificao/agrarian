# Agrarian

Agrarian is a C++ cryptocurrency-style codebase with a deterministic `depends/` build system for reproducible native and cross-compilation builds.

---

## Repository Structure

- `depends/` – deterministic third-party dependency build system
- `src/` – core source code (includes subprojects like `secp256k1/` and `univalue/`)
- `contrib/` – helper scripts, tooling, packaging, and CI utilities
- `share/` – auxiliary build and metadata scripts

---

## System Requirements (Ubuntu)

Install the baseline build tools:

```bash
sudo apt update
sudo apt install -y \
  build-essential pkg-config autoconf automake libtool \
  bsdmainutils cmake ninja-build python3 curl git
```

For Windows cross-compilation support:

```bash
sudo apt install -y mingw-w64
```

---

## Building Dependencies (Deterministic Depends System)

The preferred build path uses the deterministic `depends/` system instead of
mixing system libraries. The current native Ubuntu wallet path builds Qt 6.8.3,
OpenSSL 3.5.6, Boost 1.91.0, protobuf, Berkeley DB, and supporting libraries
inside `depends/<host-triplet>/`.

All dependency builds are executed from within the `depends/` directory or via
the helper scripts in `contrib/`.

### Native Linux

```bash
cd depends
make -j"$(nproc)"
```

### Windows Cross-Compile (64-bit)

```bash
cd depends
make HOST=x86_64-w64-mingw32 -j"$(nproc)"
```

### Windows Cross-Compile (32-bit)

```bash
cd depends
make HOST=i686-w64-mingw32 -j"$(nproc)"
```

Artifacts are placed under:

```
depends/<host-triplet>/
```

Temporary build directories:

```
depends/work/
depends/built/
```

These directories should not be committed to version control.

---

## Building Agrarian

Recommended native Ubuntu daemon build:

```bash
./autogen.sh
JOBS=1 ./contrib/build-linux.sh
```

Recommended native Ubuntu desktop wallet build:

```bash
JOBS=1 ./contrib/build-linux-wallet.sh
```

Use a larger `JOBS` value only when the host has enough RAM. On an 8-core,
16 GB host, `JOBS=8` is reasonable.

Manual autotools workflow:

```bash
./autogen.sh
CONFIG_SITE="$PWD/depends/x86_64-pc-linux-gnu/share/config.site" ./configure
make -j"$(nproc)"
```

To see configuration options:

```bash
./configure --help
```

For current build notes, see:

- `doc/modernization.md` for the current 2.0 dependency and build baseline.
- `doc/build-menu.md` for the interactive build menu.
- `doc/build-ubuntu-24.md` for native Ubuntu daemon and wallet builds.
- `doc/build-windows.md` for Windows cross-compilation.
- `doc/build-wallets.md` for the repeatable desktop wallet quick start.

---

## Running Tests

If enabled:

```bash
make check
```

Current smoke tests:

```bash
./contrib/smoke-test-daemon.sh
./contrib/smoke-test-wallet.sh
./contrib/smoke-test-qt.sh
```

---

## Development Guidelines

- Do not commit `depends/work/`, `depends/built/`, or host prefix directories.
- Ensure executable permissions (100755) are intentional for shell scripts.
- Keep diffs minimal in `depends/` to maintain reproducibility.

---

## License

See the `COPYING` file for license details.

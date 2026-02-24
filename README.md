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

Install required build tools:

```bash
sudo apt update
sudo apt install -y \
  build-essential pkg-config autoconf automake libtool \
  bsdmainutils python3 curl git
```

For Windows cross-compilation support:

```bash
sudo apt install -y mingw-w64
```

---

## Building Dependencies (Deterministic Depends System)

All dependency builds are executed from within the `depends/` directory.

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

Standard autotools workflow:

```bash
./autogen.sh
./configure
make -j"$(nproc)"
```

To see configuration options:

```bash
./configure --help
```

---

## Running Tests

If enabled:

```bash
make check
```

Refer to scripts under `contrib/` for additional CI or functional test flows.

---

## Development Guidelines

- Do not commit `depends/work/`, `depends/built/`, or host prefix directories.
- Ensure executable permissions (100755) are intentional for shell scripts.
- Keep diffs minimal in `depends/` to maintain reproducibility.

---

## License

See the `COPYING` file for license details.
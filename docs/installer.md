# Ubuntu Installer

`installer/agrarian-installer.sh` provides a Ubuntu-only CLI for common build
actions. The current modernization baseline is Ubuntu 24.04 with deterministic
`depends/`; see `doc/modernization.md` for the verified dependency set.

For fresh Ubuntu hosts, prefer `contrib/agrarian-build-menu.sh` first. The menu
script installs target-specific packages, updates the selected branch, builds
the deterministic depends tree, and can install/start the daemon for the current
user. This installer CLI is retained as a lower-level build tool.

## Usage

```bash
installer/agrarian-installer.sh [options]
```

Options:

- `--host <triplet>`: target host triplet (default: `x86_64-pc-linux-gnu`)
- `--action depends|daemon|qt|all`: build action (default: `all`)
- `--wallet 0|1`: disable/enable wallet-related build flags (default: `1`)
- `--jobs <n>`: parallel jobs (default: `nproc`)
- `--update`: run `git pull --rebase` before build
- `--reset-depends`: remove `depends/work`, `depends/built`, and `depends/<host>`
- `--yes`: skip confirmation prompts
- `--log <path>`: write a build log to the provided path

Examples:

```bash
installer/agrarian-installer.sh --action depends --host x86_64-pc-linux-gnu --yes
installer/agrarian-installer.sh --action all --wallet 0 --jobs 4 --log /tmp/agrarian-install.log
```

If wallet is enabled (default), installer preflight checks require:

- `depends/<host>/include/db_cxx.h`
- `depends/<host>/lib/libboost_thread*.a`
- `depends/<host>/lib/libboost_system*.a`

For the current native Ubuntu Qt wallet path, depends should also provide Qt6
pkg-config files such as `Qt6Core.pc`, `Qt6Gui.pc`, `Qt6Network.pc`, and
`Qt6Widgets.pc`. The Linux Qt/font path also builds Expat 2.8.0 and FreeType
2.13.3 through depends. The Windows cross-build path also uses the Qt6 depends
target.

When missing, the installer exits early and prints the exact missing path(s) plus the `make -C depends ...` command to fix them.

## Tests

Run installer tests with:

```bash
installer/tests/run.sh
```

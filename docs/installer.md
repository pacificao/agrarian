# Ubuntu Installer

`installer/agrarian-installer.sh` provides a Ubuntu-only CLI for common build actions.

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
`Qt6Widgets.pc`. The Windows cross-build path still uses the existing Qt5
depends target.

When missing, the installer exits early and prints the exact missing path(s) plus the `make -C depends ...` command to fix them.

## Tests

Run installer tests with:

```bash
installer/tests/run.sh
```

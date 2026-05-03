Agrarian 2.0 Modernization Status
=================================

This branch is the modernization line for Agrarian. The goal is to preserve
network behavior while making the codebase build repeatably on current systems.

Verified Build Baseline
-----------------------

Use Ubuntu 24.04 as the primary build host.

The current deterministic `depends/` baseline is:

- Qt 6.8.3 LTS
- OpenSSL 3.5.6 LTS
- Boost 1.91.0
- Expat 2.8.0 for the Linux Qt/font stack
- FreeType 2.14.3 for Qt 6 static font support
- Berkeley DB 4.8.30 for portable legacy wallet compatibility
- qrencode 4.1.1 for Qt wallet QR code support
- libevent 2.1.12-stable
- GMP 6.3.0
- MiniUPnPc 2.3.3 when UPnP support is enabled
- ZeroMQ 4.3.5 when enabled

Obsolete BIP70 payment request support was removed to eliminate the protobuf
and `protoc` dependency from the wallet build. Standard `agrarian:` URI payment
links are still supported.

Berkeley DB 4.8.30 is intentionally retained for legacy `wallet.dat`
compatibility. Future wallet-storage modernization should add a dedicated SQLite
backend and migration flow instead of changing the BDB version in-place.

The currently verified binaries are:

- Ubuntu daemon and command-line tools
- Ubuntu Qt desktop wallet
- Windows x86_64 Qt desktop wallet and command-line tools cross-compiled from
  Ubuntu

Recommended Commands
--------------------

Native Ubuntu daemon:

    JOBS=1 ./contrib/build-linux.sh

Native Ubuntu desktop wallet:

    JOBS=1 ./contrib/build-linux-wallet.sh

Windows x86_64 desktop wallet:

    JOBS=1 ./contrib/build-win64-wallet.sh

Use higher `JOBS` values only on hosts with enough memory. On an 8-core, 16 GB
host, `JOBS=8` is reasonable.

Verification
------------

After a native Ubuntu build, run:

    ./contrib/smoke-test-daemon.sh
    ./contrib/smoke-test-wallet.sh
    ./contrib/smoke-test-qt.sh

After a Windows cross build, confirm the expected PE binaries:

    file src/qt/agrarian-qt.exe src/agrariand.exe src/agrarian-cli.exe src/agrarian-tx.exe

Documentation Status
--------------------

The preferred docs are:

- `doc/build-ubuntu-24.md`
- `doc/build-wallets.md`
- `doc/build-windows.md`
- `doc/dependencies.md`

Older macOS, RPM, Gitian, and Travis documents remain in the tree as historical
reference. They are not part of the verified 2.0 modernization path unless a
document explicitly says otherwise.

Copyright (c) 2026 Agrarian Developers

Desktop Wallet Build Quick Start
================================

This page records the repeatable desktop wallet build flow for the modernized
Agrarian tree.

Supported targets
-----------------

The currently tested desktop wallet targets are:

* Ubuntu 24.04 native x86_64: `src/qt/agrarian-qt`
* Windows x86_64 cross build from Ubuntu: `src/qt/agrarian-qt.exe`

Use one job on small servers:

    JOBS=1 ./contrib/build-linux-wallet.sh
    JOBS=1 ./contrib/build-win64-wallet.sh

Use more jobs only when the host has enough memory. On an 8-core, 16 GB host,
`JOBS=8` is reasonable. On very small VPS instances, prefer `JOBS=1` and swap.

Ubuntu package baseline
-----------------------

    sudo apt-get update
    sudo apt-get install -y \
      build-essential pkg-config autoconf automake libtool bsdmainutils \
      curl git make tar patch

For the native Ubuntu wallet:

    sudo apt-get install -y \
      cmake ninja-build xvfb

For the Windows wallet:

    sudo apt-get install -y \
      mingw-w64 g++-mingw-w64-x86-64 g++-mingw-w64-x86-64-posix \
      qttools5-dev-tools

Then select POSIX Mingw-w64 threading:

    sudo update-alternatives --set x86_64-w64-mingw32-gcc /usr/bin/x86_64-w64-mingw32-gcc-posix
    sudo update-alternatives --set x86_64-w64-mingw32-g++ /usr/bin/x86_64-w64-mingw32-g++-posix

Why the helpers exist
---------------------

The wallet build is sensitive to tool version mismatches:

* Native Ubuntu uses the deterministic depends Qt6, OpenSSL, Boost, protobuf,
  and supporting libraries.
* Windows cross-target wallets use the deterministic depends Qt6 path and
  matching Qt host tools staged by depends.

The helper scripts keep those rules in one place so the build is repeatable on a
fresh server.

Expected artifacts
------------------

After the Ubuntu wallet build:

    src/qt/agrarian-qt
    src/agrariand
    src/agrarian-cli
    src/agrarian-tx

After the Windows wallet build:

    src/qt/agrarian-qt.exe
    src/agrariand.exe
    src/agrarian-cli.exe
    src/agrarian-tx.exe

Version checks
--------------

On Ubuntu, use the smoke tests:

    ./contrib/smoke-test-daemon.sh
    ./contrib/smoke-test-wallet.sh
    ./contrib/smoke-test-qt.sh

For Windows binaries from the Linux build host:

    file src/qt/agrarian-qt.exe src/agrariand.exe src/agrarian-cli.exe src/agrarian-tx.exe

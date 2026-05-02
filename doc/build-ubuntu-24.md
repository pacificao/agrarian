Copyright (c) 2022-2036 Agrarian Developers

Ubuntu 24.04 Build Notes
========================

These notes describe the native daemon and desktop wallet build paths tested
against current Ubuntu 24.04 system packages.

Recommended host size
---------------------

Use at least 2 GB RAM for a reliable build. A 512 MB host with 2 GB swap can
configure successfully, but compilation is extremely slow and may time out while
building large translation units.

Use `JOBS=1` on small hosts. On an 8-core, 16 GB host, `JOBS=8` is reasonable.

Packages
--------

Install the native daemon build dependencies:

    sudo apt-get update
    sudo apt-get install -y \
      build-essential pkg-config autoconf automake libtool bsdmainutils \
      cmake ninja-build python3 curl git make tar patch bzip2 xz-utils

For a native Qt wallet build and headless Qt smoke test, also install:

    sudo apt-get install -y \
      xvfb libfontconfig1-dev libfreetype-dev libharfbuzz-dev \
      libbrotli-dev libbz2-dev libexpat1-dev libglib2.0-dev \
      libgraphite2-dev libpng-dev zlib1g-dev libx11-xcb-dev \
      libxcb1-dev libxcb-cursor-dev libxcb-icccm4-dev \
      libxcb-image0-dev libxcb-keysyms1-dev libxcb-randr0-dev \
      libxcb-render0-dev libxcb-render-util0-dev libxcb-shape0-dev \
      libxcb-shm0-dev libxcb-sync-dev libxcb-util-dev \
      libxcb-xfixes0-dev libxcb-xinerama0-dev libxcb-xkb-dev \
      libxau-dev libxdmcp-dev libxext-dev libxi-dev libxrender-dev \
      libxkbcommon-dev libxkbcommon-x11-dev

Daemon build
------------

For the daemon and CLI tools, use:

    JOBS=1 ./contrib/build-linux.sh

The daemon helper builds/restores the native deterministic depends prefix first
with Qt disabled. Berkeley DB, OpenSSL, Boost, libevent, GMP, and supporting
libraries come from `depends/`, not from Ubuntu system headers.

Qt wallet build
---------------

For the desktop wallet, use:

    JOBS=1 ./contrib/build-linux-wallet.sh

The wallet helper builds/restores the native depends prefix first, including
Qt 6.8.3, OpenSSL 3.5.6, Boost 1.91.0, Expat 2.8.0, FreeType 2.13.3, and the
Linux Qt font/xcb support libraries. It then configures the project with the
generated depends `config.site` and builds the Ubuntu Qt wallet.

Defaults:

    HOST=x86_64-pc-linux-gnu

Warnings
--------

The Qt6/OpenSSL3 path should build without Qt compatibility warnings. First-run
runtime logs may report missing cache files such as `peers.dat`, `banlist.dat`,
or masternode cache files; those are created during startup.

Functional smoke test
---------------------

After building, run the isolated smoke tests:

    ./contrib/smoke-test-daemon.sh
    ./contrib/smoke-test-wallet.sh
    ./contrib/smoke-test-qt.sh

The daemon script confirms RPC and wallet calls work, mines one block, checks
the block count, and stops the daemon. The wallet script checks address
creation, mining, send/confirm, backup, and encryption shutdown. The Qt script
starts `agrarian-qt` under Xvfb and confirms the GUI reaches `Done loading`
without Qt6 TLS, signal, or stylesheet compatibility warnings.

OpenSSL 3
---------

The deterministic depends build currently uses OpenSSL 3.5.6.

Expat and FreeType
------------------

The native Ubuntu Qt wallet path uses Expat 2.8.0 through fontconfig and
FreeType 2.13.3 for Qt 6 static font support. Keep those in the deterministic
depends graph; do not satisfy them with ad hoc system-library fallbacks.

Berkeley DB
-----------

Ubuntu 24.04 provides Berkeley DB 5.3. Wallet builds with BDB 5.3 are not
portable back to BDB 4.8 wallet environments, so only use wallets created by this
build with compatible BDB 5.3 builds.

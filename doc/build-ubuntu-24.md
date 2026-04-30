Copyright (c) 2026 Agrarian Developers

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
      cmake ninja-build python3 curl git

For a headless Qt wallet smoke test, also install:

    sudo apt-get install -y \
      xvfb

Daemon-only build
-----------------

For the pool daemon, the GUI, tests, bench, ZMQ, and UPnP can be disabled:

    ./autogen.sh
    ./configure \
      --without-gui \
      --disable-tests \
      --disable-bench \
      --disable-zmq \
      --with-miniupnpc=no \
      --with-incompatible-bdb \
      CXXFLAGS="-O0 -g0 --param ggc-min-expand=1 --param ggc-min-heapsize=32768"
    make -j1

The daemon helper runs the same path:

    JOBS=1 ./contrib/build-linux.sh

Qt wallet build
---------------

For the desktop wallet, use:

    JOBS=1 ./contrib/build-linux-wallet.sh

The wallet helper builds/restores the native depends prefix first, including
Qt 6.8.3, OpenSSL 3.5.6, and Boost 1.91.0. It then configures the project with
the generated depends `config.site` and builds the Ubuntu Qt wallet.

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

Berkeley DB
-----------

Ubuntu 24.04 provides Berkeley DB 5.3. Wallet builds with BDB 5.3 are not
portable back to BDB 4.8 wallet environments, so only use wallets created by this
build with compatible BDB 5.3 builds.

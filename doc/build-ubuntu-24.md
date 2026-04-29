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

Packages
--------

Install the native daemon build dependencies:

    sudo apt-get update
    sudo apt-get install -y \
      build-essential pkg-config autoconf automake libtool bsdmainutils \
      libboost-all-dev libevent-dev libgmp-dev libssl-dev \
      libdb5.3-dev libdb5.3++-dev

For the desktop wallet, also install Qt and protobuf tools:

    sudo apt-get install -y \
      qtbase5-dev qttools5-dev-tools qtchooser \
      libqrencode-dev libprotobuf-dev protobuf-compiler

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

The wallet helper builds/restores the native depends prefix first, then creates
a temporary config.site that keeps the depends libraries while allowing system
Qt 5 tools and pkg-config files to be used.

Defaults:

    HOST=x86_64-pc-linux-gnu
    QT_BINDIR=/usr/lib/qt5/bin
    PROTOC_BINDIR=/usr/bin
    CONFIG_SITE_FILE=/tmp/agrarian-linux-wallet-config.site

Warnings
--------

Qt 5.15 and protobuf 3.x emit compatibility/deprecation warnings in a few Qt
translation units. Those warnings are expected while the Windows build remains
pinned to Qt 5.9.7 and protobuf 2.6.1.

Functional smoke test
---------------------

After building the daemon, run the isolated regtest smoke test:

    ./contrib/smoke-test-daemon.sh

The script starts `agrariand` with a temporary regtest datadir, confirms RPC and
wallet calls work, mines one block with `generate`, checks the block count, and
stops the daemon.

OpenSSL 3
---------

Ubuntu 24.04 ships OpenSSL 3. Agrarian no longer rejects this only because
`RAND_egd` is unavailable. Some deprecated OpenSSL SHA calls still warn during
compilation; those warnings are expected until the hashing helper code is
modernized.

Berkeley DB
-----------

Ubuntu 24.04 provides Berkeley DB 5.3. Wallet builds with BDB 5.3 are not
portable back to BDB 4.8 wallet environments, so only use wallets created by this
build with compatible BDB 5.3 builds.

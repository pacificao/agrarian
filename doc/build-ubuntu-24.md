Copyright (c) 2026 Agrarian Developers

Ubuntu 24.04 Daemon Build Notes
================================

These notes describe the native daemon build path tested against current Ubuntu
24.04 system packages.

Recommended host size
---------------------

Use at least 2 GB RAM for a reliable build. A 512 MB host with 2 GB swap can
configure successfully, but compilation is extremely slow and may time out while
building large translation units.

Packages
--------

Install the native build dependencies:

    sudo apt-get update
    sudo apt-get install -y \
      build-essential pkg-config autoconf automake libtool bsdmainutils \
      libboost-all-dev libevent-dev libgmp-dev libssl-dev \
      libdb5.3-dev libdb5.3++-dev

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

Copyright (c) 2026 Agrarian Developers

UNIX Build Notes

These notes describe how to build Agrarian Core on Unix-based systems.
For Ubuntu 24.04, prefer the shorter and tested path in
`doc/build-ubuntu-24.md`.

IMPORTANT

Always use absolute paths when configuring and compiling Agrarian Core
and its dependencies.

Example:

    ../dist/configure --enable-cxx --disable-shared --with-pic --prefix=$BDB_PREFIX

$BDB_PREFIX must be an absolute path. Using $(pwd) ensures an absolute
path is used.

STANDARD BUILD WITH DEPENDS

    make -C depends HOST=x86_64-pc-linux-gnu NO_QT=0 -j$(nproc)
    ./autogen.sh
    CONFIG_SITE=$PWD/depends/x86_64-pc-linux-gnu/share/config.site ./configure
    make -j$(nproc)
    make install  (optional)

The native depends wallet path currently builds Qt 6.8.3, OpenSSL 3.5.6,
Boost 1.91.0, Expat 2.8.0, FreeType 2.13.3, protobuf, Berkeley DB, and
supporting libraries.

For a daemon-only build:

    JOBS=1 ./contrib/build-linux.sh

For a native Ubuntu desktop wallet build:

    JOBS=1 ./contrib/build-linux-wallet.sh

DEPENDENCIES

Required: - libssl : Crypto (RNG, ECC) - libboost : Utility (threading,
data structures) - libevent : Networking (async networking) - libgmp :
Bignum arithmetic

Optional: - miniupnpc : UPnP support - libdb4.8 : Berkeley DB (wallet
builds only) - qt : GUI support - protobuf : GUI payment protocol -
libqrencode: QR code support - univalue : JSON parsing (bundled by
default) - libzmq3 : ZMQ notifications (>= 4.0.0)

See dependencies.md for version details.

MEMORY REQUIREMENTS

Minimum recommended: 1.5 GB RAM.

Low memory systems:

    ./configure CXXFLAGS="--param ggc-min-expand=1 --param ggc-min-heapsize=32768"

UBUNTU / DEBIAN

Build tools:

    sudo apt-get install build-essential libtool bsdmainutils autotools-dev autoconf pkg-config automake cmake ninja-build python3 curl git

System libraries are needed only for non-depends builds. The deterministic
depends build should be preferred for reproducibility.

Libraries for a system-library build:

    sudo apt-get install libssl-dev libgmp-dev libevent-dev libboost-all-dev

Berkeley DB 4.8 (wallet support):

    sudo apt-get install software-properties-common
    sudo add-apt-repository ppa:bitcoin/bitcoin
    sudo apt-get update
    sudo apt-get install libdb4.8-dev libdb4.8++-dev

Optional:

    sudo apt-get install libminiupnpc-dev
    sudo apt-get install libzmq3-dev

Qt GUI:

    Use the native depends wallet helper for Qt6:

    JOBS=1 ./contrib/build-linux-wallet.sh

Disable GUI:

    ./configure --without-gui

FEDORA

Build tools:

    sudo dnf install which gcc-c++ libtool make autoconf automake     openssl-devel libevent-devel boost-devel     libdb4-devel libdb4-cxx-devel gmp-devel python3

Optional:

    sudo dnf install miniupnpc-devel zeromq-devel

Qt:

    Native Qt6 builds are currently tested through the Ubuntu depends path.
    Fedora system-Qt builds are not part of the current verified path.

HARDENING

Enable:

    ./configure --enable-hardening

Disable:

    ./configure --disable-hardening

Verify:

    scanelf -e ./agrariand

DISABLE WALLET MODE

    ./configure --disable-wallet

ARM CROSS COMPILATION

    sudo apt-get install g++-arm-linux-gnueabihf curl

    cd depends
    make HOST=arm-linux-gnueabihf NO_QT=1
    cd ..
    ./autogen.sh
    ./configure --prefix=$PWD/depends/arm-linux-gnueabihf       --enable-glibc-back-compat --enable-reduce-exports       LDFLAGS=-static-libstdc++
    make

SMOKE TESTS

After a native build, run:

    ./contrib/smoke-test-daemon.sh
    ./contrib/smoke-test-wallet.sh
    ./contrib/smoke-test-qt.sh

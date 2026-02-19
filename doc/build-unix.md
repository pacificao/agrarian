Copyright (c) 2026 Agrarian Developers

UNIX Build Notes

These notes describe how to build Agrarian Core on Unix-based systems.

IMPORTANT

Always use absolute paths when configuring and compiling Agrarian Core
and its dependencies.

Example:

    ../dist/configure --enable-cxx --disable-shared --with-pic --prefix=$BDB_PREFIX

$BDB_PREFIX must be an absolute path. Using $(pwd) ensures an absolute
path is used.

STANDARD BUILD

    ./autogen.sh
    ./configure
    make
    make install  (optional)

If dependencies are satisfied, this will build agrarian-qt as well.

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

    sudo apt-get install build-essential libtool bsdmainutils autotools-dev autoconf pkg-config automake python3

Libraries:

    sudo apt-get install libssl-dev libgmp-dev libevent-dev libboost-all-dev

OpenSSL Note: For Ubuntu >= 18.04 or Debian >= Stretch use
libssl1.0-dev. OpenSSL 1.1 is not officially supported.

Berkeley DB 4.8 (wallet support):

    sudo apt-get install software-properties-common
    sudo add-apt-repository ppa:bitcoin/bitcoin
    sudo apt-get update
    sudo apt-get install libdb4.8-dev libdb4.8++-dev

Optional:

    sudo apt-get install libminiupnpc-dev
    sudo apt-get install libzmq3-dev

Qt GUI:

    sudo apt-get install libqt5gui5 libqt5core5a libqt5dbus5     qttools5-dev qttools5-dev-tools libprotobuf-dev protobuf-compiler

Disable GUI:

    ./configure --without-gui

FEDORA

Build tools:

    sudo dnf install which gcc-c++ libtool make autoconf automake     compat-openssl10-devel libevent-devel boost-devel     libdb4-devel libdb4-cxx-devel gmp-devel python3

Optional:

    sudo dnf install miniupnpc-devel zeromq-devel

Qt:

    sudo dnf install qt5-qttools-devel qt5-qtbase-devel protobuf-devel

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

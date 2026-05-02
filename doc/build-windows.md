Copyright (c) 2022-2036 Agrarian Developers

============================================================
                Agrarian Core – Windows Build Notes
============================================================

This document describes how to build Agrarian Core for Windows.

------------------------------------------------------------
SUPPORTED BUILD METHODS
------------------------------------------------------------

The following methods are known to work:

1. Linux (Ubuntu 24.04 tested)
   Using the Mingw-w64 cross-compilation toolchain.

2. Windows 10+
   Using Windows Subsystem for Linux (WSL) with Mingw-w64.

------------------------------------------------------------
UNTESTED / PARTIALLY TESTED OPTIONS
------------------------------------------------------------

The following may work but are not officially supported:

• Cygwin
• MSYS2
• Native Visual Studio toolchain

Contributions for these methods are welcome.

============================================================
WINDOWS SUBSYSTEM FOR LINUX (WSL)
============================================================

WSL allows running a Linux environment directly on Windows without a VM.

Requirements:
• Windows 10 (64-bit only)
• Not supported on Windows Server
• Ubuntu recommended

------------------------------------------------------------
INSTALLING WSL
------------------------------------------------------------

1. Enable WSL
   - Run: OptionalFeatures.exe
   - Enable "Windows Subsystem for Linux"
   - Restart if prompted

2. Install Ubuntu
   - Open Microsoft Store
   - Install "Ubuntu"

3. Complete Setup
   - Open command prompt
   - Launch the installed Ubuntu distribution
   - Create a UNIX user account

Once WSL is active, continue with cross-compilation instructions below.

============================================================
CROSS-COMPILATION (Ubuntu or WSL)
============================================================

The steps below work on:
• Native Ubuntu
• Ubuntu VM
• WSL

------------------------------------------------------------
GENERAL DEPENDENCIES
------------------------------------------------------------

    sudo apt update
    sudo apt install -y build-essential libtool autotools-dev \
        automake pkg-config bsdmainutils curl git make tar patch \
        cmake ninja-build python3

A host toolchain (build-essential) is required because some dependencies
(e.g., protobuf) build host utilities during the process.

If building the Windows installer (`make deploy`):

    sudo apt install nsis

------------------------------------------------------------
SOURCE CODE
------------------------------------------------------------

    git clone https://github.com/agrarian-project/agrarian.git
    cd agrarian

============================================================
BUILDING FOR 64-BIT WINDOWS
============================================================

Install Mingw-w64 toolchain:

    sudo apt install -y \
        mingw-w64 g++-mingw-w64-x86-64 g++-mingw-w64-x86-64-posix

Select the POSIX thread model (required):

    sudo update-alternatives --set x86_64-w64-mingw32-gcc \
        /usr/bin/x86_64-w64-mingw32-gcc-posix
    sudo update-alternatives --set x86_64-w64-mingw32-g++ \
        /usr/bin/x86_64-w64-mingw32-g++-posix


------------------------------------------------------------
IMPORTANT (WSL USERS)
------------------------------------------------------------

The source directory MUST reside inside the Linux filesystem
(e.g., /usr/src/agrarian).

DO NOT build from /mnt/c or any mounted Windows path.
Autoconf scripts will fail.

------------------------------------------------------------
BUILD COMMANDS
------------------------------------------------------------

Recommended portable helper:

    JOBS=1 ./contrib/build-win64-wallet.sh

The helper:

• Verifies the POSIX Mingw-w64 thread model.
• Builds the Win64 depends tree with Qt enabled.
• Stages matching Qt host tools (`moc`, `uic`, `rcc`) into the depends prefix.
• Builds/stages protobuf 2.6.1 `protoc` when system `protoc` is newer.
• Configures the wallet against the generated depends prefix.

Manual equivalent:

    PATH=$(echo "$PATH" | sed -e 's/:\/mnt.*//g')
    cd depends
    make HOST=x86_64-w64-mingw32 NO_QT=0
    cd ..
    ./autogen.sh
    CONFIG_SITE=$PWD/depends/x86_64-w64-mingw32/share/config.site \
        ./configure --prefix=/ \
          --disable-maintainer-mode \
          --disable-tests \
          --disable-bench \
          --with-gui=qt6 \
          --with-qt-incdir=$PWD/depends/x86_64-w64-mingw32/include \
          --with-qt-libdir=$PWD/depends/x86_64-w64-mingw32/lib \
          --with-qt-plugindir=$PWD/depends/x86_64-w64-mingw32/plugins \
          --with-qt-translationdir=$PWD/depends/x86_64-w64-mingw32/translations \
          --with-qt-bindir=$PWD/depends/build/x86_64-pc-linux-gnu/bin \
          --with-protoc-bindir=$PWD/depends/build/x86_64-pc-linux-gnu/bin
    make

============================================================
BUILDING FOR 32-BIT WINDOWS
============================================================

The current verified 2.0 Windows path is x86_64. The 32-bit instructions below
are retained as a legacy cross-build reference and should be revalidated before
release use.

Install toolchain:

    sudo apt install g++-mingw-w64-i686 mingw-w64-i686-dev

If multiple MinGW thread models are installed:

    sudo update-alternatives --config i686-w64-mingw32-g++

Select the POSIX thread model.

------------------------------------------------------------
BUILD COMMANDS
------------------------------------------------------------

    PATH=$(echo "$PATH" | sed -e 's/:\/mnt.*//g')
    cd depends
    make HOST=i686-w64-mingw32
    cd ..
    ./autogen.sh
    CONFIG_SITE=$PWD/depends/i686-w64-mingw32/share/config.site \
        ./configure --prefix=/
    make

============================================================
DEPENDS SYSTEM
============================================================

For additional documentation, see:

    depends/README.md

============================================================
INSTALLATION
============================================================

To install into a Windows-accessible directory:

    make install DESTDIR=/mnt/c/workspace/agrarian

To build a Windows installer:

    make deploy

============================================================
THREAD MODEL NOTE
============================================================

Ubuntu Mingw-w64 packages include two thread models:

• win32 (default)
• posix

The win32 model conflicts with certain C++11 headers
(e.g., std::mutex) used by Agrarian Core.

You MUST select the POSIX thread model when prompted by
update-alternatives.

============================================================
PROTOBUF NOTE
============================================================

The Windows depends build uses protobuf 2.6.1 headers and libraries. Do not use
a newer system `protoc` to regenerate wallet sources for this target. The helper
builds and stages a matching native `protoc` when needed.

============================================================
QT NOTE
============================================================

The Windows cross-build helper targets the Qt 6.8 LTS depends path. Keep the
helper's Qt host tools (`moc`, `uic`, `rcc`) staged from the matching depends
build to avoid mixing host Qt tools with target Qt libraries.

============================================================
DEPENDS BASELINE NOTE
============================================================

The 2.0 branch currently verifies Windows x86_64 builds against the same
modernized core dependency baseline as Linux: OpenSSL 3.5.6, Boost 1.91.0,
Qt 6.8.3, Berkeley DB 4.8.30, protobuf 2.6.1, libevent 2.1.8-stable, and GMP
6.1.2. Expat 2.8.0 and FreeType 2.13.3 are part of the Linux Qt/font depends
path and are documented in `doc/dependencies.md`.

============================================================
END OF DOCUMENT
============================================================

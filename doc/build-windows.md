Copyright (c) 2026 Agrarian Developers

============================================================
                Agrarian Core – Windows Build Notes
============================================================

This document describes how to build Agrarian Core for Windows.

------------------------------------------------------------
SUPPORTED BUILD METHODS
------------------------------------------------------------

The following methods are known to work:

1. Linux (Ubuntu 18.04 Bionic recommended)
   Using the Mingw-w64 cross-compilation toolchain.
   This is the method used to produce official Windows release binaries.

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
• Ubuntu recommended (tested on Ubuntu 18.04)

------------------------------------------------------------
INSTALLING WSL
------------------------------------------------------------

1. Enable WSL
   - Run: OptionalFeatures.exe
   - Enable "Windows Subsystem for Linux"
   - Restart if prompted

2. Install Ubuntu
   - Open Microsoft Store
   - Install "Ubuntu 18.04"

3. Complete Setup
   - Open command prompt
   - Run: Ubuntu1804
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
    sudo apt upgrade
    sudo apt install build-essential libtool autotools-dev \
        automake pkg-config bsdmainutils curl git

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

    sudo apt install g++-mingw-w64-x86-64

Ubuntu 18.04:

    sudo update-alternatives --config x86_64-w64-mingw32-g++

Select the POSIX thread model (required).

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

    PATH=$(echo "$PATH" | sed -e 's/:\/mnt.*//g')
    cd depends
    make HOST=x86_64-w64-mingw32
    cd ..
    ./autogen.sh
    CONFIG_SITE=$PWD/depends/x86_64-w64-mingw32/share/config.site \
        ./configure --prefix=/
    make

============================================================
BUILDING FOR 32-BIT WINDOWS
============================================================

Install toolchain:

    sudo apt install g++-mingw-w64-i686 mingw-w64-i686-dev

Ubuntu 18.04:

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
END OF DOCUMENT
============================================================

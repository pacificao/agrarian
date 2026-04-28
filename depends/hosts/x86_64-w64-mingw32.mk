# Copyright (c) 2026 Agrarian Developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or https://www.opensource.org/licenses/mit-license.php.

host_arch := x86_64
host_os   := mingw32

host_prefix := $(BASEDIR)/$(HOST)
build_prefix := $(BASEDIR)/build/$(BUILD)

x86_64_mingw32_host := $(HOST)
x86_64_mingw32_prefix := $(host_prefix)
x86_64_mingw32_id_string := $(HOST)

include hosts/default.mk
include hosts/mingw32.mk

x86_64_mingw32_WINDRES := $(HOST)-windres

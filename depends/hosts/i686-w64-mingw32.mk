# Copyright (c) 2026 Agrarian Developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or https://www.opensource.org/licenses/mit-license.php.

host_arch := x86_64
host_os   := linux

host_prefix := $(BASEDIR)/$(HOST)
build_prefix := $(BASEDIR)/build/$(BUILD)

x86_64_linux_host := $(BUILD)
x86_64_linux_prefix := $(host_prefix)
x86_64_linux_id_string := $(HOST)

include hosts/default.mk
include hosts/linux.mk

x86_64_linux_CC := gcc -m64
x86_64_linux_CXX := g++ -m64
x86_64_linux_AR := ar
x86_64_linux_RANLIB := ranlib
x86_64_linux_NM := nm
x86_64_linux_STRIP := strip

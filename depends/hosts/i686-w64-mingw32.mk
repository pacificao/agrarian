# Copyright (c) 2022-2036 Agrarian Developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or https://www.opensource.org/licenses/mit-license.php.

host_arch := i686
host_os   := mingw32

host_prefix := $(BASEDIR)/$(HOST)
build_prefix := $(BASEDIR)/build/$(BUILD)

i686_mingw32_host := $(HOST)
i686_mingw32_prefix := $(host_prefix)
i686_mingw32_id_string := $(HOST)

include hosts/default.mk
include hosts/mingw32.mk

i686_mingw32_WINDRES := $(HOST)-windres

# aarch64 Linux native build (Ubuntu 22.04/24.04 arm64)
host_arch := aarch64
host_os   := linux

host_prefix := $(BASEDIR)/$(HOST)
build_prefix := $(BASEDIR)/build/$(BUILD)

aarch64_linux_host   := $(HOST)
aarch64_linux_prefix := $(host_prefix)
aarch64_linux_id_string := $(HOST)

include hosts/default.mk
include hosts/linux.mk

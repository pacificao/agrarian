# x86_64 Linux native build (Ubuntu 22.04/24.04)
host_arch := x86_64
host_os   := linux

# Where the final host prefix will live (this directory will be created at depends/<host>)
# This must NOT be empty.
host_prefix := $(BASEDIR)/$(HOST)

# Native/build tools prefix
build_prefix := $(BASEDIR)/build/$(BUILD)

# Tell depends what “type” means for this host
x86_64_linux_host   := $(HOST)
x86_64_linux_prefix := $(host_prefix)

# Build system identity string (used in build-id hashing)
x86_64_linux_id_string := $(HOST)

# Pull in defaults + linux host settings
include hosts/default.mk
include hosts/linux.mk
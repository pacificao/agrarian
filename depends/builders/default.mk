default_build_CC = gcc
default_build_CXX = g++
default_build_AR = ar
default_build_RANLIB = ranlib
default_build_STRIP = strip
default_build_NM = nm
default_build_OTOOL = otool
default_build_INSTALL_NAME_TOOL = install_name_tool

define add_build_tool_func
build_$(build_os)_$1 ?= $$(default_build_$1)
build_$(build_arch)_$(build_os)_$1 ?= $$(build_$(build_os)_$1)
build_$1=$$(build_$(build_arch)_$(build_os)_$1)
endef
$(foreach var,CC CXX AR RANLIB NM STRIP SHA256SUM DOWNLOAD OTOOL INSTALL_NAME_TOOL,$(eval $(call add_build_tool_func,$(var))))
define add_build_flags_func
build_$(build_arch)_$(build_os)_$1 += $(build_$(build_os)_$1)
build_$1=$$(build_$(build_arch)_$(build_os)_$1)
endef
$(foreach flags, CFLAGS CXXFLAGS LDFLAGS, $(eval $(call add_build_flags_func,$(flags))))

# -----------------------------------------------------------------------------
# Network / download defaults
# -----------------------------------------------------------------------------
#
# Some older depends trees relied on these being provided via the environment.
# On newer distros, passing curl options without numeric values will hard-fail
# (e.g. "--connect-timeout" or "--retry" with an empty argument).

DOWNLOAD_CONNECT_TIMEOUT ?= 30
DOWNLOAD_RETRIES ?= 3

# Generic source mirror used when a primary upstream is down.
# You can override this at make time: make FALLBACK_DOWNLOAD_PATH=...
FALLBACK_DOWNLOAD_PATH ?= https://bitcoincore.org/depends-sources

# Packages manifest for Agrarian depends system
# Derived from Bitcoin Core depends layout, adapted for this codebase.
#
# This file defines:
#   - packages:        the set of packages built for the target HOST
#   - native_packages: build-machine tools needed during the build
#   - all_packages:    union of both
#
# Feature toggles (override on make command line if needed):
#   NO_QT=1        -> disable Qt GUI deps
#   USE_WALLET=1   -> enable BerkeleyDB wallet deps
#   USE_ZMQ=1      -> enable ZeroMQ deps
#   USE_UPNP=1     -> enable miniupnpc deps

# ---- Base packages (always) ----
packages := boost openssl libevent gmp

# ---- Feature toggles (defaults) ----
NO_QT       ?= 1
USE_WALLET  ?= 1
USE_ZMQ     ?= 0
USE_UPNP    ?= 1

# ---- Group definitions (as you had them) ----
qt_native_packages := native_protobuf
qt_packages := qrencode protobuf zlib

qt_linux_packages := qt expat dbus libxcb xcb_proto libXau xproto freetype fontconfig libX11 xextproto libXext xtrans
qt_darwin_packages := qt
qt_mingw32_packages := qt

wallet_packages := bdb
zmq_packages := zeromq
upnp_packages := miniupnpc

darwin_native_packages := native_biplist native_ds_store native_mac_alias
ifneq ($(build_os),darwin)
darwin_native_packages += native_cctools native_cdrkit native_libdmg-hfsplus
endif

# ---- Fold optional groups into 'packages' and 'native_packages' ----
native_packages :=

# Qt (and Qt-native tools)
ifeq ($(NO_QT),0)
  packages += $(qt_packages)
  native_packages += $(qt_native_packages)

  # Host OS specific Qt dependency set
  ifeq ($(host_os),linux)
    packages += $(qt_linux_packages)
  endif
  ifeq ($(host_os),darwin)
    packages += $(qt_darwin_packages)
    native_packages += $(darwin_native_packages)
  endif
  ifeq ($(host_os),mingw32)
    packages += $(qt_mingw32_packages)
  endif
else
  # Even if Qt is off, darwin native tools may still be required for packaging
  ifeq ($(host_os),darwin)
    native_packages += $(darwin_native_packages)
  endif
endif

# Wallet / BerkeleyDB
ifeq ($(USE_WALLET),1)
  packages += $(wallet_packages)
endif

# ZeroMQ
ifeq ($(USE_ZMQ),1)
  packages += $(zmq_packages)
endif

# UPnP
ifeq ($(USE_UPNP),1)
  packages += $(upnp_packages)
endif

# ---- Final sets ----
all_packages := $(sort $(packages) $(native_packages))
packages := $(sort $(packages))
native_packages := $(sort $(native_packages))

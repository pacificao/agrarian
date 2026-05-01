PACKAGE=qt
$(package)_version=6.8.3
$(package)_download_path=https://download.qt.io/official_releases/qt/6.8/$($(package)_version)/submodules
$(package)_suffix=everywhere-src-$($(package)_version).tar.xz
$(package)_file_name=qtbase-$($(package)_suffix)
$(package)_sha256_hash=56001b905601bb9023d399f3ba780d7fa940f3e4861e496a7c490331f49e0b80
$(package)_dependencies=openssl zlib
$(package)_linux_dependencies=
$(package)_build_subdir=qtbase-build

$(package)_qttranslations_file_name=qttranslations-$($(package)_suffix)
$(package)_qttranslations_sha256_hash=c3c61d79c3d8fe316a20b3617c64673ce5b5519b2e45535f49bee313152fa531

$(package)_qttools_file_name=qttools-$($(package)_suffix)
$(package)_qttools_sha256_hash=02a4e219248b94f1333df843d25763f35251c1074cdc4fb5bda67d340f8c8b3a

$(package)_extra_sources  = $($(package)_qttranslations_file_name)
$(package)_extra_sources += $($(package)_qttools_file_name)

define $(package)_set_vars
$(package)_config_opts_release = -release
$(package)_config_opts_debug = -debug
$(package)_config_opts += -bindir bin
$(package)_config_opts += -confirm-license
$(package)_config_opts += -no-cups
$(package)_config_opts += -no-dbus
$(package)_config_opts += -no-glib
$(package)_config_opts += -no-icu
$(package)_config_opts += -no-opengl
$(package)_config_opts += -no-pch
$(package)_config_opts += -no-feature-sql
$(package)_config_opts += -no-feature-vulkan
$(package)_config_opts += -nomake examples
$(package)_config_opts += -nomake tests
$(package)_config_opts += -opensource
$(package)_config_opts += -openssl-linked
$(package)_config_opts += -pkg-config
$(package)_config_opts += -prefix $(host_prefix)
$(package)_config_opts += -qt-harfbuzz
$(package)_config_opts += -qt-libjpeg
$(package)_config_opts += -qt-libpng
$(package)_config_opts += -qt-pcre
$(package)_config_opts += -static
$(package)_config_opts += -system-zlib
$(package)_config_opts_linux  = -fontconfig
$(package)_config_opts_linux += -qpa xcb
$(package)_config_opts_linux += -xcb
$(package)_config_opts_linux += -xkbcommon
$(package)_config_opts_linux += -feature-xkbcommon-x11
$(package)_config_opts_linux += -system-freetype
$(package)_config_opts_linux += -no-feature-sessionmanager
$(package)_config_opts_mingw32 = -qpa windows
$(package)_config_opts_darwin = -qpa cocoa
$(package)_cmake_opts_mingw32  = -DCMAKE_SYSTEM_NAME=Windows
$(package)_cmake_opts_mingw32 += -DCMAKE_C_COMPILER=$($(package)_cc)
$(package)_cmake_opts_mingw32 += -DCMAKE_CXX_COMPILER=$($(package)_cxx)
$(package)_cmake_opts_mingw32 += -DCMAKE_RC_COMPILER=$(host_toolchain)windres
$(package)_cmake_opts_mingw32 += -DOPENSSL_ROOT_DIR=$(host_prefix)
$(package)_cmake_opts_mingw32 += -DOPENSSL_USE_STATIC_LIBS=TRUE
$(package)_cmake_opts_mingw32 += -DZLIB_ROOT=$(host_prefix)
$(package)_cmake_opts_mingw32 += -DQT_HOST_PATH=$(BASEDIR)/$(BUILD)
$(package)_cmake_opts_mingw32 += -DCMAKE_FIND_ROOT_PATH=$(host_prefix)
$(package)_cmake_opts_mingw32 += -DCMAKE_FIND_ROOT_PATH_MODE_PACKAGE=ONLY
$(package)_cmake_opts_mingw32 += -DCMAKE_FIND_ROOT_PATH_MODE_LIBRARY=ONLY
$(package)_cmake_opts_mingw32 += -DCMAKE_FIND_ROOT_PATH_MODE_INCLUDE=ONLY
$(package)_cmake_opts_mingw32 += -DCMAKE_FIND_ROOT_PATH_MODE_PROGRAM=NEVER
endef

define $(package)_fetch_cmds
$(call fetch_file,$(package),$($(package)_download_path),$($(package)_download_file),$($(package)_file_name),$($(package)_sha256_hash)) && \
$(call fetch_file,$(package),$($(package)_download_path),$($(package)_qttranslations_file_name),$($(package)_qttranslations_file_name),$($(package)_qttranslations_sha256_hash)) && \
$(call fetch_file,$(package),$($(package)_download_path),$($(package)_qttools_file_name),$($(package)_qttools_file_name),$($(package)_qttools_sha256_hash))
endef

define $(package)_extract_cmds
  mkdir -p $($(package)_extract_dir) && \
  echo "$($(package)_sha256_hash)  $($(package)_source)" > $($(package)_extract_dir)/.$($(package)_file_name).hash && \
  echo "$($(package)_qttranslations_sha256_hash)  $($(package)_source_dir)/$($(package)_qttranslations_file_name)" >> $($(package)_extract_dir)/.$($(package)_file_name).hash && \
  echo "$($(package)_qttools_sha256_hash)  $($(package)_source_dir)/$($(package)_qttools_file_name)" >> $($(package)_extract_dir)/.$($(package)_file_name).hash && \
  $(build_SHA256SUM) -c $($(package)_extract_dir)/.$($(package)_file_name).hash && \
  mkdir qtbase qtbase-build qttools qttools-build qttranslations qttranslations-build && \
  tar --no-same-owner --strip-components=1 -xf $($(package)_source) -C qtbase && \
  tar --no-same-owner --strip-components=1 -xf $($(package)_source_dir)/$($(package)_qttools_file_name) -C qttools && \
  tar --no-same-owner --strip-components=1 -xf $($(package)_source_dir)/$($(package)_qttranslations_file_name) -C qttranslations
endef

define $(package)_config_cmds
    qt_system_pc="$$$${QT_SYSTEM_PKG_CONFIG_LIBDIR:-`unset PKG_CONFIG_LIBDIR PKG_CONFIG_PATH; pkg-config --variable pc_path pkg-config 2>/dev/null || true`}" && \
    if test -z "$$$${qt_system_pc}"; then \
      qt_multiarch="`gcc -print-multiarch 2>/dev/null || true`"; \
      if test -n "$$$${qt_multiarch}"; then \
        qt_system_pc="/usr/lib/$$$${qt_multiarch}/pkgconfig:/lib/$$$${qt_multiarch}/pkgconfig:/usr/lib/pkgconfig:/usr/share/pkgconfig"; \
      else \
        qt_system_pc="/usr/lib/pkgconfig:/usr/share/pkgconfig"; \
      fi; \
    fi && \
    export PKG_CONFIG_SYSROOT_DIR=/ && \
    export PKG_CONFIG_LIBDIR=$(host_prefix)/lib/pkgconfig$(if $(filter linux,$(host_os)),:$$$${qt_system_pc}) && \
    export PKG_CONFIG_PATH=$(host_prefix)/share/pkgconfig && \
  ../qtbase/configure $($(package)_config_opts) -- -G Ninja $($(package)_cmake_opts) $($(package)_cmake_opts_$(host_os)) $($(package)_cmake_opts_$(host_arch)_$(host_os))
endef

define $(package)_build_cmds
  cmake --build . --parallel
endef

ifeq ($(host_os),mingw32)
define $(package)_stage_cmds
  DESTDIR=$($(package)_staging_dir) cmake --install .
endef
else
define $(package)_stage_cmds
  DESTDIR=$($(package)_staging_dir) cmake --install . && \
  mkdir -p ../qttools-build && \
  cd ../qttools-build && \
  cmake -G Ninja ../qttools \
    -DCMAKE_PREFIX_PATH=$($(package)_staging_prefix_dir) \
    -DCMAKE_INSTALL_PREFIX=$(host_prefix) \
    -DQT_HOST_PATH=$($(package)_staging_prefix_dir) \
    -DQT_BUILD_EXAMPLES=FALSE \
    -DQT_BUILD_TESTS=FALSE \
    -DBUILD_SHARED_LIBS=OFF && \
  cmake --build . --target lrelease --parallel && \
  mkdir -p $($(package)_staging_prefix_dir)/libexec && \
  cp ../qttools-build/bin/lrelease $($(package)_staging_prefix_dir)/libexec/lrelease
endef
endif

define $(package)_postprocess_cmds
  rm -rf share/doc share/examples share/qt6/sbom && \
  rm -f lib/lib*.la lib/*.prl plugins/*/*.prl
endef

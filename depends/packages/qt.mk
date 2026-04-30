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
$(package)_config_opts_linux += -system-freetype
$(package)_config_opts_linux += -no-feature-sessionmanager
$(package)_config_opts_mingw32 = -qpa windows
$(package)_config_opts_darwin = -qpa cocoa
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
    export PKG_CONFIG_SYSROOT_DIR=/ && \
    export PKG_CONFIG_LIBDIR=$(host_prefix)/lib/pkgconfig:/usr/lib/x86_64-linux-gnu/pkgconfig:/usr/share/pkgconfig && \
    export PKG_CONFIG_PATH=$(host_prefix)/share/pkgconfig && \
  ../qtbase/configure $($(package)_config_opts) -- -G Ninja
endef

define $(package)_build_cmds
  cmake --build . --parallel
endef

define $(package)_stage_cmds
  DESTDIR=$($(package)_staging_dir) cmake --install .
endef

define $(package)_postprocess_cmds
  rm -rf lib/cmake share/doc share/examples share/qt6/sbom && \
  rm -f lib/lib*.la lib/*.prl plugins/*/*.prl
endef

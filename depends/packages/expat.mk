package=expat
$(package)_version=2.8.0
$(package)_download_path=https://github.com/libexpat/libexpat/releases/download/R_2_8_0/
$(package)_file_name=$(package)-$($(package)_version).tar.bz2
$(package)_sha256_hash=586494499ac3ad46d87f3beda7b1f770c1c8026a9b60e151593f8b29089a52ca

define $(package)_set_vars
$(package)_config_opts=--disable-static --without-docbook
endef

define $(package)_config_cmds
  $($(package)_autoconf)
endef

define $(package)_build_cmds
  $(MAKE)
endef

define $(package)_stage_cmds
  $(MAKE) DESTDIR=$($(package)_staging_dir) install
endef

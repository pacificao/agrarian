package=native_protobuf
$(package)_version=2.6.1
$(package)_download_path=https://github.com/google/protobuf/releases/download/v$($(package)_version)
$(package)_file_name=protobuf-$($(package)_version).tar.bz2
$(package)_sha256_hash=ee445612d544d885ae240ffbcbf9267faa9f593b7b101f21d58beceb92661910

define $(package)_set_vars
$(package)_config_opts=--disable-shared --without-zlib
endef

define $(package)_config_cmds
  $($(package)_autoconf)
endef

define $(package)_build_cmds
  $(MAKE) -C src protoc
endef

define $(package)_stage_cmds
  mkdir -p $($(package)_staging_prefix_dir)/bin && \
  cp src/protoc $($(package)_staging_prefix_dir)/bin/protoc && \
  $(build_STRIP) $($(package)_staging_prefix_dir)/bin/protoc
endef

define $(package)_postprocess_cmds
  test -x bin/protoc || \
    (echo "ERROR: native_protobuf staged without bin/protoc" && find . -maxdepth 3 -type f -o -type d && exit 1) && \
  chmod +x bin/protoc
endef

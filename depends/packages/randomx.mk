package=randomx
$(package)_version=1.2.3
$(package)_download_path=https://github.com/tevador/RandomX/archive/refs/tags
$(package)_file_name=v$($(package)_version).tar.gz
$(package)_sha256_hash=096af0bbe9d0dfeed6962c8541cd8f705a7ee2cfc2a6f433bcecc56941b720b8

define $(package)_set_vars
  $(package)_cmake_opts=-DCMAKE_BUILD_TYPE=Release
  $(package)_cmake_opts+=-DARCH=default
  $(package)_cmake_opts+=-DBUILD_SHARED_LIBS=OFF
ifeq ($(host_arch)_$(host_os),x86_64_mingw32)
$(package)_cmake_opts+=-DARCH_ID=x86_64
endif
endef

define $(package)_preprocess_cmds
  mkdir build
endef

define $(package)_config_cmds
  cd build && $($(package)_cmake) ..
endef

define $(package)_build_cmds
  $(MAKE) -C build randomx
endef

define $(package)_stage_cmds
  mkdir -p $($(package)_staging_prefix_dir)/include $($(package)_staging_prefix_dir)/lib && \
  install src/randomx.h $($(package)_staging_prefix_dir)/include/randomx.h && \
  install build/librandomx.a $($(package)_staging_prefix_dir)/lib/librandomx.a
endef

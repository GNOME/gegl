project('libnsgif',
  'c',
  meson_version: '>=0.57.0',
  version: '1.0.0',
)


libnsgif_lib = static_library('nsgif',
  'gif.c',
  'lzw.c',
)


libnsgif = declare_dependency(
  link_with: libnsgif_lib,
  include_directories: '.'
)

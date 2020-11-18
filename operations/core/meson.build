
gegl_core_sources = files(
  'cache.c',
  'cast-format.c',
  'cast-space.c',
  'clone.c',
  'convert-format.c',
  'convert-space.c',
  'crop.c',
  'json.c',
  'load.c',
  'nop.c',
)

gegl_core_sources += custom_target('module_core.c',
  input : gegl_core_sources,
  output: 'module_core.c',
  command: [ gen_loader, '@INPUT@', ],
  capture: true
)

gegl_core = shared_library('gegl-core',
  gegl_core_sources,
  include_directories: [ rootInclude, geglInclude, ],
  dependencies: [
    babl,
    glib,
    gmodule,
    json_glib,
    math,
  ],
  link_with: [
    gegl_lib,
  ],
  c_args: [
    '-DGEGL_OP_BUNDLE',
  ],
  name_prefix: '',
  install: true,
  install_dir: get_option('libdir') / api_name,
)

gegl_operations += gegl_core

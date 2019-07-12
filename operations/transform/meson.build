gegl_transformops_sources = files(
  'module.c',
  'reflect.c',
  'rotate-on-center.c',
  'rotate.c',
  'scale-ratio.c',
  'scale-size-keepaspect.c',
  'scale-size.c',
  'scale.c',
  'shear.c',
  'transform-core.c',
  'transform.c',
  'translate.c',
)

gegl_transformops = shared_library('transformops',
  gegl_transformops_sources,
  include_directories: [ rootInclude, geglInclude, ],
  dependencies: [
    babl,
    glib,
    json_glib,
    math,
  ],
  link_with: [
    gegl_lib,
  ],
  name_prefix: '',
  install: true,
  install_dir: get_option('libdir') / api_name,
)

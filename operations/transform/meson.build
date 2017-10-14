operations_transform_sources = [
  'transform-core.c',
  'transform-core.h',
  'module.c',
  'module.h',
  'reflect.c',
  'rotate.c',
  'rotate-on-center.c',
  'scale-ratio.c',
  'scale-size.c',
  'scale-size-keepaspect.c',
  'shear.c',
  'transform.c',
  'translate.c',
]

operations_transform = library('gegl-transform',
  operations_transform_sources,
  include_directories: config_h_dir,
  dependencies: libgegl_dep,
  install: true,
  install_dir: operations_ext_dir,
)

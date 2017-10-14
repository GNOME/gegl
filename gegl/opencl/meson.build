libcl_sources = [
  'gegl-cl.c',
  'gegl-cl-init.c',
  'gegl-cl-color.c',
  'gegl-cl-introspection-support.h',
  'gegl-cl-random.c',
]

libcl = static_library('cl',
  libcl_sources,
  include_directories: libgegl_common_includes,
  dependencies: libgegl_common_dependencies,
)

libprocess_sources = [
  'gegl-eval-manager.c',
  'gegl-graph-traversal.c',
  'gegl-graph-traversal-debug.c',
  'gegl-list-visitor.c',
  'gegl-processor.c',
]

libprocess = static_library('process',
  libprocess_sources,
  include_directories: libgegl_common_includes,
  dependencies: libgegl_common_dependencies,
)

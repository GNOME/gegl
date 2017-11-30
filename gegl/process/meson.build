gegl_process_sources = files(
  'gegl-eval-manager.c',
  'gegl-graph-traversal-debug.c',
  'gegl-graph-traversal.c',
  'gegl-processor.c',
)

gegl_process_lib = static_library('gegl_process',
  gegl_process_sources,
  include_directories: [ rootInclude, geglInclude, ],
  dependencies: [
    babl,
    glib,
  ],
)

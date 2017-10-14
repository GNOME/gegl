libgraph_sources = [
  'gegl-connection.c',
  'gegl-node.c',
  'gegl-pad.c',
  'gegl-visitable.c',
  'gegl-visitor.c',
]

libgraph = static_library('graph',
  libgraph_sources,
  include_directories: libgegl_common_includes,
  dependencies: libgegl_common_dependencies,
)

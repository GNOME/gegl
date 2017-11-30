gegl_graph_sources = files(
  'gegl-callback-visitor.c',
  'gegl-connection.c',
  'gegl-node-output-visitable.c',
  'gegl-node.c',
  'gegl-pad.c',
  'gegl-visitable.c',
  'gegl-visitor.c',
)

gegl_graph_lib = static_library('gegl_graph',
  gegl_graph_sources,
  include_directories: [ rootInclude, geglInclude, ],
  dependencies: [
    babl,
    glib,
  ],
)

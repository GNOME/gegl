libgegl_sc_sources = [
  'sc-context.c',
  'sc-outline.c',
  'sc-sample.c',
]

libgegl_sc_dependencies = [
  common_deps,
  libgegl_dep,
  poly2tri_c,
]

libgegl_sc = library('gegl-sc-@0@'.format(gegl_api_version),
  libgegl_sc_sources,
  dependencies: libgegl_sc_dependencies,
)

# XXX pkg-config file

libgegl_npd_sources = [
  'npd_common.c',
  'deformation.c',
  'npd_math.c',
  'graphics.c',
  'lattice_cut.c',
  'npd_debug.c',
  'npd_gegl.c',
]

libgegl_npd_dependencies = [
  common_deps,
  libgegl_dep,
]

libgegl_npd = library('gegl-npd-@0@'.format(gegl_api_version),
  libgegl_npd_sources,
  dependencies: libgegl_npd_dependencies,
)

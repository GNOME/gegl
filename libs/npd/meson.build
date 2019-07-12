gegl_npd_sources = files(
  'deformation.c',
  'graphics.c',
  'lattice_cut.c',
  'npd_common.c',
  'npd_debug.c',
  'npd_gegl.c',
  'npd_math.c',
)

gegl_npd_headers = files(
  'deformation.h',
  'graphics.h',
  'lattice_cut.h',
  'npd_common.h',
  'npd_debug.h',
  'npd_gegl.h',
  'npd_math.h',
  'npd.h',
)

gegl_npd_lib = library(meson.project_name() + '-npd-' + api_version,
  gegl_npd_sources,
  dependencies: [ glib, babl, math, ],
  include_directories: geglInclude,
  link_with: [
    gegl_lib,
  ],
  install: true,
)

libnpd = declare_dependency(
  include_directories: include_directories('..'),
  link_with: gegl_npd_lib,
)

install_headers(gegl_npd_headers,
  subdir: api_name / 'npd',
)

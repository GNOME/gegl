
seamlessInclude = include_directories('.')

seamlessclone_sources = [
  'sc-context.c',
  'sc-outline.c',
  'sc-sample.c',
]

seamlessclone_headers = [
  'sc-common.h',
  'sc-context.h',
  'sc-outline.h',
  'sc-sample.h',
]

seamlessclone_lib = library('gegl-sc-' + api_version,
  seamlessclone_sources,
  include_directories: [ rootInclude, geglInclude, ],
  dependencies: [
    babl,
    glib,
    gobject,
    poly2tri_c,
  ],
  link_with: [
    gegl_lib,
  ],
  install: true,
)

install_headers(
  seamlessclone_headers,
  subdir: join_paths(lib_name, 'sc')
)


pkgconfig.generate(
  filebase: 'gegl-sc-' + api_version,
  name: 'GEGL Seamless Cloning',
  description: 'Seamless Cloning Library Based on GEGL',
  version: meson.project_version(),
  requires: [
    'babl',
    'gegl-' + api_version,
    'gio-2.0',
    'gio-unix-2.0',
    'gmodule-2.0',
    'gobject-2.0',
    'json-glib-1.0',
    'poly2tri-c',
  ],
  libraries: [
    seamlessclone_lib
  ],
  subdirs: [
    join_paths(lib_name, 'sc'),
  ],
)

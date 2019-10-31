
geglInclude = include_directories(
  '.',
  'buffer',
  'graph',
  'module',
  'opencl',
  'operation',
  'process',
  'property-types',
)

gegl_cflags = [
  '-DLIBDIR="@0@"'.format(get_option('prefix') / get_option('libdir')),
  '-DG_LOG_DOMAIN="@0@"'.format('GEGL'),
  '-DGEGL_LOCALEDIR="@0@"'.format(get_option('prefix') / get_option('localedir')),
]

gegl_version_h = configure_file(
  input : 'gegl-version.h.in',
  output: 'gegl-version.h',
  configuration: config,
)

gegl_introspectable_headers = files(
  'gegl.h',
  'gegl-apply.h',
  'gegl-enums.h',
  'gegl-init.h',
  'gegl-lookup.h',
  'gegl-matrix.h',
  'gegl-operations-util.h',
  'gegl-parallel.h',
  'gegl-random.h',
  'gegl-types.h',
  'gegl-utils.h',
) + [
  gegl_version_h,
]

gegl_headers = files(
  'gegl-cpuaccel.h',
  'gegl-debug.h',
  'gegl-op.h',
  'gegl-plugin.h',
)

gegl_sources = files(
  'gegl-apply.c',
  'gegl-config.c',
  'gegl-cpuaccel.c',
  'gegl-dot-visitor.c',
  'gegl-dot.c',
  'gegl-enums.c',
  'gegl-gio.c',
  'gegl-init.c',
  'gegl-instrument.c',
  'gegl-introspection-support.c',
  'gegl-lookup.c',
  'gegl-matrix.c',
  'gegl-parallel.c',
  'gegl-random.c',
  'gegl-serialize.c',
  'gegl-stats.c',
  'gegl-utils.c',
  'gegl-xml.c',
)

subdir('buffer')
subdir('graph')
subdir('module')
subdir('opencl')
subdir('operation')
subdir('process')
subdir('property-types')

gegl_headers += gegl_introspectable_headers

install_headers(gegl_headers,
  subdir: api_name
)

gegl_ldflags = os_osx ? ['-framework', 'OpenCL'] : []

gegl_lib = library(api_name,
  gegl_sources,
  include_directories: [ rootInclude, geglInclude, ],
  dependencies: [
    babl,
    glib,
    gio,
    math,
    gmodule,
  ],
  c_args: [ gegl_cflags, ],
  link_args: [ gegl_ldflags, ],
  install: true,
  version: so_version,
)


introspection_sources = gegl_introspectable_headers + files(
  'gegl-introspection-support.h',
  'opencl' / 'gegl-cl-introspection-support.h',
)


if get_option('introspection')

  gegl_gir = gnome.generate_gir(gegl_lib,
    sources: introspection_sources,
    nsversion: api_version,
    namespace: 'Gegl',
    identifier_prefix: 'Gegl',
    symbol_prefix: 'gegl',
    header: 'gegl.h',
    includes: [ 'GLib-2.0', 'GObject-2.0', 'Babl-0.1' ],
    install: true,
  )

  if vapigen.found()
     gnome.generate_vapi(api_name,
       metadata_dirs: '.',
       sources: gegl_gir[0],
       install: true,
     )
  endif
endif

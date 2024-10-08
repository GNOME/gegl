gegl_library_build_dir = meson.current_build_dir()

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
  '-DGEGL_LOCALEDIR="@0@"'.format(get_option('localedir')),
  '-DGEGL_PREFIX="@0@"'.format(get_option('prefix')),
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
  'gegl-metadata.h',
  'gegl-metadatastore.h',
  'gegl-metadatahash.h',
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
  'gegl-math.h',
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
  'gegl-metadata.c',
  'gegl-metadatastore.c',
  'gegl-metadatahash.c',
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

opencl_dep = declare_dependency(
  sources : [opencl_headers],
)


if host_cpu_family == 'x86_64'
  simd_extra = [lib_gegl_x86_64_v2, lib_gegl_x86_64_v3]
elif host_cpu_family == 'arm'
  simd_extra = [lib_gegl_arm_neon]
else
  simd_extra = []
endif

gegl_lib = library(api_name,
  gegl_sources,
  include_directories: [rootInclude, geglInclude],
  dependencies: [
    babl,
    glib,
    gio,
    math,
    gmodule,
    opencl_dep,
  ],
  c_args: gegl_cflags,

  link_with: simd_extra,
  link_args: gegl_ldflags,
  install: true,
  version: so_version,
)


if g_ir.found()
  introspection_sources = gegl_introspectable_headers + files(
    'gegl-introspection-support.h',
    'opencl' / 'gegl-cl-introspection-support.h',
  )

  if meson.version().version_compare('>=1.2')
    gegl_gir = gnome.generate_gir(gegl_lib,
      sources: introspection_sources,
      nsversion: api_version,
      namespace: 'Gegl',
      identifier_prefix: 'Gegl',
      symbol_prefix: 'gegl',
      header: 'gegl.h',
      includes: [ 'GLib-2.0', 'GObject-2.0', 'Babl-0.1' ],
      dependencies: [babl],
      export_packages: 'gegl-' + api_version,
      install: true,
      env: ['GEGL_BUILD=1'],
    )
  else
    gegl_gir = gnome.generate_gir(gegl_lib,
      sources: introspection_sources,
      nsversion: api_version,
      namespace: 'Gegl',
      identifier_prefix: 'Gegl',
      symbol_prefix: 'gegl',
      header: 'gegl.h',
      includes: [ 'GLib-2.0', 'GObject-2.0', 'Babl-0.1' ],
      dependencies: [babl],
      export_packages: 'gegl-' + api_version,
      install: true,
    )
  endif

  if vapigen.found()
    configure_file(
      input: 'Gegl.metadata.in',
      output: 'Gegl-@0@.metadata'.format(api_version),
      copy: true,
    )

    gnome.generate_vapi(api_name,
      metadata_dirs: meson.current_source_dir(),
      sources: gegl_gir[0],
      install: true,
      packages: [
        'glib-2.0',
        'gobject-2.0',
        'babl-0.1'
      ],
    )
  endif
endif

# Version header
libgegl_version_conf = configuration_data()
libgegl_version_conf.set('GEGL_MAJOR_VERSION', gegl_major_version)
libgegl_version_conf.set('GEGL_MINOR_VERSION', gegl_minor_version)
libgegl_version_conf.set('GEGL_MICRO_VERSION', gegl_micro_version)
libgegl_version_header = configure_file(
  input: 'gegl-version.h.in',
  output: 'gegl-version.h',
  configuration: libgegl_version_conf,
)

# Common includes
libgegl_common_includes = [
  config_h_dir,
  include_directories('.'),
  include_directories('buffer'),
  include_directories('graph'),
  include_directories('module'),
  include_directories('opencl'),
  include_directories('operation'),
  include_directories('process'),
  include_directories('property-types'),
]

# Common dependencies
libgegl_common_dependencies = [
  common_deps,
  libmath,
  babl,
]

# XXX G_LOG_DOMAIN

# Subdirectory
subdir('buffer')
subdir('graph')
subdir('module')
subdir('operation')
subdir('process')
subdir('property-types')
subdir('opencl')

# libgegl
libgegl_sources = [
  'gegl-algorithms.c',
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
  'gegl-random.c',
  'gegl-serialize.c',
  'gegl-utils.c',
  'gegl-xml.c',
]

libgegl_linkedlibs = [
  libbuffer,
  libmodule,
  liboperation,
  libgraph,
  libprocess,
  libpropertytypes,
  libcl,
]

libgegl = library('gegl-@0@'.format(gegl_api_version),
  libgegl_sources,
  include_directories: libgegl_common_includes,
  dependencies: libgegl_common_dependencies,
  link_with: libgegl_linkedlibs,
  install: true,
)

libgegl_dep = declare_dependency(
  link_with: [ libgegl, libgegl_linkedlibs ],
  dependencies: libgegl_common_dependencies,
  include_directories: libgegl_common_includes,
)


# Introspection
libgegl_introspectable_headers = [
  libgegl_version_header,

  'gegl-apply.h',
  'gegl-enums.h',
  'gegl-init.h',
  'gegl-lookup.h',
  'gegl-matrix.h',
  'gegl-random.h',
  'gegl-types.h',
  'gegl-utils.h',
  'gegl.h',

  'buffer/gegl-buffer-backend.h',
  'buffer/gegl-buffer-iterator.h',
  'buffer/gegl-buffer.h',
  'buffer/gegl-tile-backend.h',
  'buffer/gegl-tile-handler.h',
  'buffer/gegl-tile-source.h',

  'graph/gegl-node.h',

  'process/gegl-graph-debug.h',
  'process/gegl-processor.h',

  'property-types/gegl-audio-fragment.h',
  'property-types/gegl-color.h',
  'property-types/gegl-curve.h',
  'property-types/gegl-paramspecs.h',
  'property-types/gegl-path.h',
]

libgegl_gir_sources = [
  libgegl_introspectable_headers,
  'gegl-introspection-support.h',
  'opencl/gegl-cl-introspection-support.h',
]

libgegl_gir_includes = [
  'GObject-2.0',
  'GLib-2.0',
]

libgegl_gir = gnome.generate_gir(libgegl,
  namespace: 'Gegl',
  nsversion: gegl_api_version,
  sources: libgegl_gir_sources,
  includes: libgegl_gir_includes,
  install: true,
)


# VAPI
libgegl_vapi = gnome.generate_vapi('Gegl-@0@'.format(gegl_api_version),
  sources: libgegl_gir[0],
  metadata_dirs: [ '.' ],
  install: true,
)

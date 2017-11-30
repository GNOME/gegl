
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
  '-DLIBDIR="'        + get_option('libdir') + '"',
  '-DG_LOG_DOMAIN="'  + 'GEGL' + '"',
  '-DGEGL_LOCALEDIR="'+ get_option('localedir') + '"',
]



gegl_version_h = configure_file(
  input : 'gegl-version.h.in',
  output: 'gegl-version.h',
  configuration: conf,
  install: true,
  install_dir: join_paths(get_option('includedir'), lib_name),
)


subdir('buffer')
subdir('graph')
subdir('module')
subdir('opencl')
subdir('operation')
subdir('process')
subdir('property-types')



gegl_introspectable_headers = [
  'gegl.h',
  gegl_version_h,
  'buffer/gegl-buffer-backend.h',
  'buffer/gegl-buffer-iterator.h',
  'buffer/gegl-buffer.h',
  'buffer/gegl-tile-backend.h',
  'buffer/gegl-tile-handler.h',
  'buffer/gegl-tile-source.h',
  'gegl-apply.h',
  'gegl-enums.h',
  'gegl-init.h',
  'gegl-lookup.h',
  'gegl-matrix.h',
  'gegl-operations-util.h',
  'gegl-random.h',
  'gegl-types.h',
  'gegl-utils.h',
  'graph/gegl-node.h',
  'process/gegl-graph-debug.h',
  'process/gegl-processor.h',
  'property-types/gegl-audio-fragment.h',
  'property-types/gegl-color.h',
  'property-types/gegl-curve.h',
  'property-types/gegl-paramspecs.h',
  'property-types/gegl-path.h',
]

gegl_headers = [
  gegl_introspectable_headers,
  'buffer/gegl-buffer-cl-iterator.h',
  'buffer/gegl-tile.h',
  'gegl-cpuaccel.h',
  'gegl-debug.h',
  'gegl-op.h',
  'gegl-plugin.h',
]

gegl_sources = [
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

install_headers(gegl_headers,
  subdir: lib_name
)

gegl_lib = library(lib_name,
  gegl_sources,
  include_directories: [ rootInclude, geglInclude, ],
  dependencies: [
    babl,
    glib,
    gio,
    math,
    gmodule,
  ],
  link_whole: [
    gegl_buffer_lib,
    gegl_graph_lib,
    gegl_module_lib,
    gegl_opencl_lib,
    gegl_operation_lib,
    gegl_process_lib,
    gegl_property_types_lib,
  ],
  c_args: [ gegl_cflags, ],
  install: true,
  version: so_version,
)


introspection_sources = [
  gegl_introspectable_headers,
  'gegl-introspection-support.h',
  join_paths('opencl', 'gegl-cl-introspection-support.h'),
]

gegl_gir = gnome.generate_gir(gegl_lib,
  sources: introspection_sources,
  nsversion: api_version,
  namespace: 'Gegl',
  identifier_prefix: 'Gegl',
  symbol_prefix: 'gegl',
  header: 'gegl.h',
  includes: [ 'GLib-2.0', 'GObject-2.0', ],
  include_directories: [
    rootInclude,
    geglInclude,
  ],
  install: true,
)

if get_option('with-vala')
   gnome.generate_vapi('gegl-' + api_version,
     metadata_dirs: '.',
     sources: gegl_gir[0],
     install: true,
   )
endif


libraries = [
  'average',
  'blend-reflect',
  'negation',
  'soft-burn',
  'soft-dodge',
  'subtractive',
]

foreach lib : libraries
  shared_library(lib,
    lib + '.c',
    include_directories: [ rootInclude, geglInclude, seamlessInclude, ],
    dependencies: [
      babl,
      glib,
      gobject,
    ],
    link_with: [
      gegl_lib,
    ],
    name_prefix: '',
    install: true,
    install_dir: join_paths(get_option('libdir'), lib_name),
  )
endforeach

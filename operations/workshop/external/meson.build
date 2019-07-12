
if lua.found()
  shared_library('gluas',
    'gluas.c',
    include_directories: [ rootInclude, geglInclude, seamlessInclude, ],
    dependencies: [
      babl,
      glib,
      gobject,
      lua,
      math,
    ],
    link_with: [
      gegl_lib,
    ],
    name_prefix: '',
    install: true,
    install_dir: get_option('libdir') / api_name,
  )
endif

if lensfun.found()
  shared_library('lens-correct',
    'lens-correct.c',
    include_directories: [ rootInclude, geglInclude, seamlessInclude, ],
    dependencies: [
      babl,
      glib,
      gobject,
      lensfun,
      math,
    ],
    link_with: [
      gegl_lib,
    ],
    name_prefix: '',
    install: true,
    install_dir: get_option('libdir') / api_name,
  )
endif

if cairo.found()
  shared_library('line-profile',
    'line-profile.c',
    include_directories: [ rootInclude, geglInclude, seamlessInclude, ],
    dependencies: [
      babl,
      cairo,
      glib,
      gobject,
      math,
    ],
    link_with: [
      gegl_lib,
    ],
    name_prefix: '',
    install: true,
    install_dir: get_option('libdir') / api_name,
  )
endif

if libv4l2.found()
  shared_library('v4l2',
    'v4l2.c',
    include_directories: [ rootInclude, geglInclude, seamlessInclude, ],
    dependencies: [
      babl,
      glib,
      gobject,
      libv4l2,
      math,
    ],
    link_with: [
      gegl_lib,
    ],
    name_prefix: '',
    install: true,
    install_dir: get_option('libdir') / api_name,
  )
endif

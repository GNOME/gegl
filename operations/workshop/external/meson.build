
if get_option('with-lua')
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
    install_dir: join_paths(get_option('libdir'), lib_name),
  )
endif

if get_option('with-lensfun')
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
    install_dir: join_paths(get_option('libdir'), lib_name),
  )
endif

if get_option('with-cairo')
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
    install_dir: join_paths(get_option('libdir'), lib_name),
  )
endif

if get_option('with-libv4l2')
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
    install_dir: join_paths(get_option('libdir'), lib_name),
  )
endif



gegl_operations += shared_library('voroni_diagram',
  'voronoi-diagram.cc',
    include_directories: [ rootInclude, geglInclude, seamlessInclude, ],
    dependencies: [
      babl,
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


if lua.found()
  gegl_operations += shared_library('gluas',
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
  gegl_operations += shared_library('lens-correct',
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
  gegl_operations += shared_library('line-profile',
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
  gegl_operations += shared_library('v4l2',
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

if maxflow.found()
  gegl_operations += shared_library('paint-select',
    'paint-select.cc',
    include_directories: [ rootInclude, geglInclude, seamlessInclude, ],
    dependencies: [
      babl,
      glib,
      gobject,
      math,
      maxflow,
    ],
    link_with: [
      gegl_lib,
    ],
    name_prefix: '',
    install: true,
    install_dir: get_option('libdir') / api_name,
  )
endif

gegl_operations+=shared_library('ctx-script',
   'ctx-script.c', include_directories: [rootInclude, geglInclude],
   dependencies:[babl,glib,gobject,math,libgegl_ctx],
   link_with:[gegl_lib],
   name_prefix:'',
   install:true,
   install_dir: get_option('libdir') / api_name,
)

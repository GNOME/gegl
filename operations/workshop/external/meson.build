

gegl_operations += shared_module('voroni_diagram',
  'voronoi-diagram.cc',
    include_directories: geglWorkshopInclude
    dependencies: gegl_operations_common_deps,
    link_with: [
      gegl_lib,
    ],
    name_prefix: '',
    install: true,
    install_dir: get_option('libdir') / api_name,
)


if lua.found()
  gegl_operations += shared_module('gluas',
    'gluas.c',
    include_directories: geglWorkshopInclude
    dependencies: gegl_operations_common_deps + lua,
    link_with: [
      gegl_lib,
    ],
    name_prefix: '',
    install: true,
    install_dir: get_option('libdir') / api_name,
  )
endif

if lensfun.found()
  gegl_operations += shared_module('lens-correct',
    'lens-correct.c',
    include_directories: geglWorkshopInclude
    dependencies: gegl_operations_common_deps + lensfun,
    link_with: [
      gegl_lib,
    ],
    name_prefix: '',
    install: true,
    install_dir: get_option('libdir') / api_name,
  )
endif

if cairo.found()
  gegl_operations += shared_module('line-profile',
    'line-profile.c',
    include_directories: geglWorkshopInclude
    dependencies: gegl_operations_common_deps + cairo,
    link_with: [
      gegl_lib,
    ],
    name_prefix: '',
    install: true,
    install_dir: get_option('libdir') / api_name,
  )
endif

if libv4l2.found()
  gegl_operations += shared_module('v4l2',
    'v4l2.c',
    include_directories: geglWorkshopInclude
    dependencies: gegl_operations_common_deps + libv4l2,
    link_with: [
      gegl_lib,
    ],
    name_prefix: '',
    install: true,
    install_dir: get_option('libdir') / api_name,
  )
endif

if maxflow.found()
  gegl_operations += shared_module('paint-select',
    'paint-select.cc',
    include_directories: geglWorkshopInclude
    dependencies: gegl_operations_common_deps + maxflow,
    link_with: [
      gegl_lib,
    ],
    name_prefix: '',
    install: true,
    install_dir: get_option('libdir') / api_name,
  )
endif

gegl_operations+=shared_module('ctx-script',
   'ctx-script.c',
   include_directories: geglOperationsInclude,
   dependencies: gegl_operations_common_deps + libgegl_ctx,
   link_with:[gegl_lib],
   name_prefix:'',
   install:true,
   install_dir: get_option('libdir') / api_name,
)

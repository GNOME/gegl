gegl_operations_build_dir = meson.current_build_dir()

geglOperationsInclude = [
  rootInclude,
  geglInclude,
  include_directories('.'),
]

gegl_operations_utils_sources = files(
  'utils/gegl-op-utils-gio.c',
)

gegl_operations_utils = static_library('gegl-op-utils',
  gegl_operations_utils_sources,
  dependencies: [ gio ],
  include_directories: [ 'utils' ],
  c_args: gegl_cflags,
)

gegl_operations_utils_dep = declare_dependency(
  include_directories: [ 'utils' ],
  link_with: gegl_operations_utils,
)

gegl_operations = []
gegl_operations_common_deps = [
  babl,
  glib,
  gmodule,
  json_glib,
  math,
  gegl_operations_utils_dep,
]

# This is commented on purpose because the directory gets configured before
# even configuring 'gegl'. This is because GEGL lib itself uses some OpenCL
# kernels that are defined here. So, until those are handled in some different
# way, this cannot be uncommented. Instead, this gets called directly from the
# root meson.build file.
# subdir('opencl')

subdir('common-gpl3+')
subdir('common-cxx')
subdir('common')
subdir('core')
subdir('external')
subdir('generated')
subdir('json')
subdir('seamless-clone')
subdir('transform')
if get_option('workshop')
  subdir('workshop')
endif

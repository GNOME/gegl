
gegl_operations_build_dir = meson.current_build_dir()
gegl_operations = []

# This is commented on purpose because the directory gets configured before
# even configuring 'gegl'. This is because GEGL lib itself uses some OpenCL
# kernels that are defined here. So, until those are handled in some different
# way, this cannot be uncommented. Instead, this gets called directly from the
# root meson.build file.
# subdir('opencl')

geglOperationsInclude = [
  rootInclude,
  geglInclude,
  include_directories('.'),
]

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

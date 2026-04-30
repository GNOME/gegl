gegl_transformops_sources = files(
  'module.c',
  'border-align.c',
  'reflect.c',
  'reset-origin.c',
  'rotate-on-center.c',
  'rotate.c',
  'scale-ratio.c',
  'scale-size-keepaspect.c',
  'scale-size.c',
  'scale.c',
  'shear.c',
  'transform-core.c',
  'transform.c',
  'translate.c',
)

gegl_transformops = shared_module('transformops',
  gegl_transformops_sources,
  include_directories: geglOperationsInclude,
  dependencies: gegl_operations_common_deps,
  link_with: [
    gegl_lib,
  ],
  name_prefix: '',
  install: true,
  install_dir: get_option('libdir') / api_name,
)

gegl_operations += gegl_transformops

if host_cpu_family == 'x86_64'

  gegl_transformops_x86_64_v2 = shared_module('gegl-transformops-x86_64-v2',
    gegl_transformops_sources, gegl_operations_opencl_headers,
    include_directories: geglOperationsInclude,
    dependencies: gegl_operations_common_deps,
    link_with: [ gegl_lib, ],
    c_args: x86_64_v2_flags,
    name_prefix: '',
    install: true,
    install_dir: get_option('libdir') / api_name,
  )
  gegl_operations += gegl_transformops_x86_64_v2

  gegl_transformops_x86_64_v3 = shared_module('gegl-transformops-x86_64-v3',
    gegl_transformops_sources, gegl_operations_opencl_headers,
    include_directories: geglOperationsInclude,
    dependencies: gegl_operations_common_deps,
    link_with: [ gegl_lib, ],
    c_args: x86_64_v3_flags,
    name_prefix: '',
    install: true,
    install_dir: get_option('libdir') / api_name,
  )
  gegl_operations += gegl_transformops_x86_64_v3

elif host_cpu_family == 'arm'

  gegl_transformops_arm_neon = shared_module('gegl-transformops-arm-neon',
    gegl_transformops_sources, gegl_operations_opencl_headers,
    include_directories: geglOperationsInclude,
    dependencies: gegl_operations_common_deps,
    link_with: [ gegl_lib, ],
    c_args: arm_neon_flags,
    name_prefix: '',
    install: true,
    install_dir: get_option('libdir') / api_name,
  )
  gegl_operations += gegl_transformops_arm_neon

endif

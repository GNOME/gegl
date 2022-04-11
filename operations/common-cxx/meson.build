
gegl_common_cxx_sources = files(
  'denoise-dct.cc',
  'distance-transform.cc',
  'focus-blur.c',
  'lens-blur.cc',
  'piecewise-blend.cc',
  'variable-blur.c',
  'warp.cc',
)

gegl_common_cxx_sources += custom_target('module_common_gpl3.c',
  input : [ gegl_common_cxx_sources ],
  output: [ 'module_common_gpl3.c' ],
  command: [
    gen_loader,
    '@INPUT@',
  ],
  capture: true
)

gegl_common_cxx = shared_library('gegl-common-cxx',
  [ gegl_common_cxx_sources, opencl_headers, ],
  include_directories: [ rootInclude, geglInclude, ],
  dependencies: [
    babl,
    glib,
    json_glib,
    math,
  ],
  link_with: [
    gegl_lib,
  ],
  c_args: [   '-DGEGL_OP_BUNDLE', ],
  cpp_args: [ '-DGEGL_OP_BUNDLE', ],
  name_prefix: '',
  install: true,
  install_dir: get_option('libdir') / api_name,
)

gegl_operations += gegl_common_cxx

if host_cpu_family == 'x86_64'

  gegl_common_cxx_x86_64_v2 = shared_library('gegl-common-cxx-x86_64-v2',
    gegl_common_cxx_sources, opencl_headers,
    include_directories: [ rootInclude, geglInclude, ],
    dependencies: [
      babl,
      glib,
      json_glib,
      math,
    ],
    link_with: [
      gegl_lib,
    ],
    c_args: [ '-DGEGL_OP_BUNDLE' ] + x86_64_v2_flags,
    cpp_args: [ '-DGEGL_OP_BUNDLE' ] + x86_64_v2_flags,
    name_prefix: '',
    install: true,
    install_dir: get_option('libdir') / api_name,
  )
  gegl_operations += gegl_common_cxx_x86_64_v2
  
  gegl_common_cxx_x86_64_v3 = shared_library('gegl-common-cxx-x86_64-v3',
    gegl_common_cxx_sources, opencl_headers,
    include_directories: [ rootInclude, geglInclude, ],
    dependencies: [
      babl,
      glib,
      json_glib,
      math,
    ],
    link_with: [
      gegl_lib,
    ],
    c_args: [ '-DGEGL_OP_BUNDLE' ] + x86_64_v3_flags,
    cpp_args: [ '-DGEGL_OP_BUNDLE' ] + x86_64_v3_flags,
    name_prefix: '',
    install: true,
    install_dir: get_option('libdir') / api_name,
  )
  gegl_operations += gegl_common_cxx_x86_64_v3

elif host_cpu_family == 'arm'

  gegl_common_cxx_arm_neon = shared_library('gegl-common-cxx-arm-neon',
    gegl_common_cxx_sources, opencl_headers,
    include_directories: [ rootInclude, geglInclude, ],
    dependencies: [
      babl,
      glib,
      json_glib,
      math,
    ],
    link_with: [
      gegl_lib,
    ],
    c_args: [ '-DGEGL_OP_BUNDLE' ] + arm_neon_flags,
    cpp_args: [ '-DGEGL_OP_BUNDLE' ] + arm_neon_flags,
    name_prefix: '',
    install: true,
    install_dir: get_option('libdir') / api_name,
  )
  gegl_operations += gegl_common_cxx_arm_neon

endif

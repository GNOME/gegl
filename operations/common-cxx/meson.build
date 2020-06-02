
gegl_common_cxx_sources = files(
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

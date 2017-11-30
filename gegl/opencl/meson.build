gegl_opencl_sources = files(
  'gegl-cl.c',
  'gegl-cl-init.c',
  'gegl-cl-color.c',
  'gegl-cl-random.c',
)

gegl_opencl_headers = files(
  'cl_d3d10.h',
  'cl_ext.h',
  'cl_gl_ext.h',
  'cl_gl.h',
  'cl_platform.h',
  'cl.h',
  'gegl-cl-color.h',
  'gegl-cl-init.h',
  'gegl-cl-random.h',
  'gegl-cl-types.h',
  'gegl-cl.h',
  'opencl.h',
)

install_headers(gegl_opencl_headers,
  subdir: join_paths(lib_name, 'opencl')
)

gegl_opencl_lib = static_library('gegl_opencl',
  [ gegl_opencl_sources, opencl_headers, ],
  include_directories: [ rootInclude, geglInclude, ],
  dependencies: [
    babl,
    glib,
  ],
)

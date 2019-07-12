gegl_sources += files(
  'gegl-buffer-cl-cache.c',
  'gegl-buffer-cl-iterator.c',
  'gegl-cl-color.c',
  'gegl-cl-init.c',
  'gegl-cl-random.c',
  'gegl-cl.c',
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
  subdir: api_name / 'opencl'
)

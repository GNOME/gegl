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
  'cl_d3d11.h',
  'cl_dx9_media_sharing.h',
  'cl_egl.h',
  'cl_ext.h',
  'cl_gl.h',
  'cl_half.h',
  'cl_icd.h',
  'cl_layer.h',
  'cl_platform.h',
  'cl_va_api_media_sharing_intel.h',
  'cl_version.h',
  'cl.h',
  'gegl-cl-color.h',
  'gegl-cl-init.h',
  'gegl-cl-random.h',
  'gegl-cl-types.h',
  'gegl-cl-version.h',
  'gegl-cl.h',
  'opencl.h',
  'opencl.hpp',
)

install_headers(gegl_opencl_headers,
  subdir: api_name / 'opencl'
)

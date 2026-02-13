gegl_sources += files(
  'gegl-buffer-cl-cache.c',
  'gegl-buffer-cl-iterator.c',
  'gegl-cl-color.c',
  'gegl-cl-init.c',
  'gegl-cl-random.c',
  'gegl-cl.c',
)

upstream_opencl_headers = files (
  'CL/cl.h',
  'CL/cl_d3d10.h',
  'CL/cl_d3d11.h',
  'CL/cl_dx9_media_sharing.h',
  'CL/cl_dx9_media_sharing_intel.h',
  'CL/cl_egl.h',
  'CL/cl_ext.h',
  'CL/cl_ext_intel.h',
  'CL/cl_function_types.h',
  'CL/cl_gl.h',
  'CL/cl_gl_ext.h',
  'CL/cl_half.h',
  'CL/cl_icd.h',
  'CL/cl_layer.h',
  'CL/cl_platform.h',
  'CL/cl_va_api_media_sharing_intel.h',
  'CL/cl_version.h',
  'CL/opencl.h',
  'CL/opencl.hpp',
)

install_headers(upstream_opencl_headers,
  subdir: api_name / 'opencl' / 'CL'
)

gegl_opencl_headers = files(
  'gegl-cl-color.h',
  'gegl-cl-init.h',
  'gegl-cl-random.h',
  'gegl-cl-types.h',
  'gegl-cl-version.h',
  'gegl-cl.h',
)

install_headers(gegl_opencl_headers,
  subdir: api_name / 'opencl'
)

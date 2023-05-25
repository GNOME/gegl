
gegl_ctx_lib = static_library('gegl_ctx',
  'gegl_ctx.c',
  'ctx.h',
  dependencies: [glib, babl],
  include_directories: rootInclude,
)

libgegl_ctx = declare_dependency(
  include_directories: include_directories('.'),
  link_with: gegl_ctx_lib,
)


gegl_ctx_lib_cflags = []
if cc.get_argument_syntax() == 'msvc'
  gegl_ctx_lib_cflags = [
    #Allow unsafe functions on ctx-only
    #to reduce friction with ctx development
    '/D_CRT_SECURE_NO_WARNINGS',
  ]
endif

gegl_ctx_lib = static_library('gegl_ctx',
  'gegl_ctx.c',
  'ctx.h',
  dependencies: [glib, babl],
  include_directories: rootInclude,
  c_args: gegl_ctx_lib_cflags,
)

libgegl_ctx = declare_dependency(
  include_directories: include_directories('.'),
  link_with: gegl_ctx_lib,
)

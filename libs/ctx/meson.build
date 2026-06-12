
gegl_ctx_lib_cflags = []
if cc.get_argument_syntax() == 'msvc'
  gegl_ctx_lib_cflags = [
    #Allow unsafe functions on ctx-only
    #to reduce friction with ctx development
    '/D_CRT_SECURE_NO_WARNINGS',
  ]
endif

gegl_ctx_lib_sources = [
  'gegl_ctx.c',
]
libgegl_ctx_include = [rootInclude]
if not cc.has_header('ctx.h')
  #vendored ctx.h (on libgegl_ctx subdir to avoid unconditional including)
  message('using built-in ctx.h')
  gegl_ctx_lib_sources += ['libgegl_ctx/ctx.h']
  libgegl_ctx_include += ['./libgegl_ctx']
endif

gegl_ctx_lib = static_library('gegl_ctx',
  gegl_ctx_lib_sources,
  dependencies: [glib, babl],
  include_directories: libgegl_ctx_include,
  c_args: gegl_ctx_lib_cflags,
)

libgegl_ctx = declare_dependency(
  include_directories: libgegl_ctx_include,
  link_with: gegl_ctx_lib,
)
